#include <iostream>
#include <string>
#include <unordered_map>
#include <thread>
#include <vector>

#include "compilation_database_loader.h"
#include "indexer.h"
#include "ipc.h"
#include "query.h"
#include "language_server_api.h"
#include "test.h"

#include "src/typed_bidi_message_queue.h"

#include "third_party/tiny-process-library/process.hpp"

#include "third_party/doctest/doctest/doctest.h"

#include <rapidjson/istreamwrapper.h>
#include <rapidjson/ostreamwrapper.h>

#ifdef _WIN32
#include <io.h>
#include <fcntl.h>
#endif

const char* kIpcIndexerName = "indexer";
const char* kIpcLanguageClientName = "language_client";

const int kNumIndexers = 8 - 1;

std::unordered_map<std::string, std::string> ParseOptions(int argc,
                                                          char** argv) {
  std::unordered_map<std::string, std::string> output;

  std::string previous_arg;

  for (int i = 1; i < argc; ++i) {
    std::string arg = argv[i];

    if (arg[0] != '-') {
      if (previous_arg.size() == 0) {
        std::cerr << "Invalid arguments; switches must start with -"
                  << std::endl;
        exit(1);
      }

      output[previous_arg] = arg;
      previous_arg = "";
    } else {
      output[arg] = "";
      previous_arg = arg;
    }
  }

  return output;
}

bool HasOption(const std::unordered_map<std::string, std::string>& options,
               const std::string& option) {
  return options.find(option) != options.end();
}

std::unique_ptr<InMessage> ParseMessage() {
  int content_length = -1;
  int iteration = 0;
  while (true) {
    if (++iteration > 10) {
      assert(false && "bad parser state");
      exit(1);
    }

    std::string line;
    std::getline(std::cin, line);
    // std::cin >> line;

    if (line.compare(0, 14, "Content-Length") == 0) {
      content_length = atoi(line.c_str() + 16);
    }

    if (line == "\r")
      break;
  }

  // bad input that is not a message.
  if (content_length < 0) {
    std::cerr << "parsing command failed (no Content-Length header)"
              << std::endl;
    return nullptr;
  }

  std::string content;
  content.reserve(content_length);
  for (int i = 0; i < content_length; ++i) {
    char c;
    std::cin >> c;
    content += c;
  }

  rapidjson::Document document;
  document.Parse(content.c_str(), content_length);
  assert(!document.HasParseError());

  return MessageRegistry::instance()->Parse(document);
}

std::string Join(const std::vector<std::string>& elements, std::string sep) {
  bool first = true;
  std::string result;
  for (const auto& element : elements) {
    if (!first)
      result += ", ";
    first = false;
    result += element;
  }
  return result;
}

template <typename T>
struct BaseIpcMessage : public IpcMessage {
  BaseIpcMessage() : IpcMessage(T::kIpcId) {}

  // IpcMessage:
  void Serialize(Writer& writer) override {
    T& value = *static_cast<T*>(this);
    Reflect(writer, value);
  }
  void Deserialize(Reader& reader) override {
    T& value = *static_cast<T*>(this);
    Reflect(reader, value);
  }
};

struct IpcMessage_Quit : public BaseIpcMessage<IpcMessage_Quit> {
  static constexpr IpcId kIpcId = IpcId::Quit;
};
template <typename TVisitor>
void Reflect(TVisitor& visitor, IpcMessage_Quit& value) {}

struct IpcMessage_IsAlive : public BaseIpcMessage<IpcMessage_IsAlive> {
  static constexpr IpcId kIpcId = IpcId::IsAlive;
};
template <typename TVisitor>
void Reflect(TVisitor& visitor, IpcMessage_IsAlive& value) {}

struct IpcMessage_OpenProject : public BaseIpcMessage<IpcMessage_OpenProject> {
  static constexpr IpcId kIpcId = IpcId::OpenProject;
  std::string project_path;
};
template <typename TVisitor>
void Reflect(TVisitor& visitor, IpcMessage_OpenProject& value) {
  Reflect(visitor, value.project_path);
}

struct IpcMessage_IndexTranslationUnitRequest
    : public BaseIpcMessage<IpcMessage_IndexTranslationUnitRequest> {
  static constexpr IpcId kIpcId = IpcId::IndexTranslationUnitRequest;
  std::string path;
  std::vector<std::string> args;
};
template <typename TVisitor>
void Reflect(TVisitor& visitor, IpcMessage_IndexTranslationUnitRequest& value) {
  REFLECT_MEMBER_START();
  REFLECT_MEMBER(path);
  REFLECT_MEMBER(args);
  REFLECT_MEMBER_END();
}

struct IpcMessage_IndexTranslationUnitResponse
    : public BaseIpcMessage<IpcMessage_IndexTranslationUnitResponse> {
  static constexpr IpcId kIpcId = IpcId::IndexTranslationUnitResponse;
  IndexUpdate update;

  IpcMessage_IndexTranslationUnitResponse() {}
  explicit IpcMessage_IndexTranslationUnitResponse(IndexUpdate& update)
      : update(update) {}
};
template <typename TVisitor>
void Reflect(TVisitor& visitor,
             IpcMessage_IndexTranslationUnitResponse& value) {
  REFLECT_MEMBER_START();
  REFLECT_MEMBER(update);
  REFLECT_MEMBER_END();
}

struct IpcMessage_LanguageServerRequest
    : public BaseIpcMessage<IpcMessage_LanguageServerRequest> {
  static constexpr IpcId kIpcId = IpcId::LanguageServerRequest;
  // TODO: provide a way to get the request state.
  lsMethodId method_id;
};
template <typename TVisitor>
void Reflect(TVisitor& visitor, IpcMessage_LanguageServerRequest& value) {
  REFLECT_MEMBER_START();
  REFLECT_MEMBER(method_id);
  REFLECT_MEMBER_END();
}

struct IpcMessage_DocumentSymbolsRequest
    : public BaseIpcMessage<IpcMessage_DocumentSymbolsRequest> {
  static constexpr IpcId kIpcId = IpcId::DocumentSymbolsRequest;
  RequestId request_id;
  std::string document;
};
template <typename TVisitor>
void Reflect(TVisitor& visitor, IpcMessage_DocumentSymbolsRequest& value) {
  REFLECT_MEMBER_START();
  REFLECT_MEMBER(request_id);
  REFLECT_MEMBER(document);
  REFLECT_MEMBER_END();
}

struct IpcMessage_DocumentSymbolsResponse
    : public BaseIpcMessage<IpcMessage_DocumentSymbolsResponse> {
  static constexpr IpcId kIpcId = IpcId::DocumentSymbolsResponse;
  RequestId request_id;
  std::vector<lsSymbolInformation> symbols;
};
template <typename TVisitor>
void Reflect(TVisitor& visitor, IpcMessage_DocumentSymbolsResponse& value) {
  REFLECT_MEMBER_START();
  REFLECT_MEMBER(request_id);
  REFLECT_MEMBER(symbols);
  REFLECT_MEMBER_END();
}

struct IpcMessage_WorkspaceSymbolsRequest
    : public BaseIpcMessage<IpcMessage_WorkspaceSymbolsRequest> {
  static constexpr IpcId kIpcId = IpcId::WorkspaceSymbolsRequest;
  RequestId request_id;
  std::string query;
};
template <typename TVisitor>
void Reflect(TVisitor& visitor, IpcMessage_WorkspaceSymbolsRequest& value) {
  REFLECT_MEMBER_START();
  REFLECT_MEMBER(request_id);
  REFLECT_MEMBER(query);
  REFLECT_MEMBER_END();
}

struct IpcMessage_DocumentCodeLensRequest
    : public BaseIpcMessage<IpcMessage_DocumentCodeLensRequest> {
  static constexpr IpcId kIpcId = IpcId::DocumentCodeLensRequest;
  RequestId request_id;
  std::string document;
};
template <typename TVisitor>
void Reflect(TVisitor& visitor, IpcMessage_DocumentCodeLensRequest& value) {
  REFLECT_MEMBER_START();
  REFLECT_MEMBER(request_id);
  REFLECT_MEMBER(document);
  REFLECT_MEMBER_END();
}

struct IpcMessage_DocumentCodeLensResponse
    : public BaseIpcMessage<IpcMessage_DocumentCodeLensResponse> {
  static constexpr IpcId kIpcId = IpcId::DocumentCodeLensResponse;
  RequestId request_id;
  std::vector<TCodeLens> code_lens;
};
template <typename TVisitor>
void Reflect(TVisitor& visitor, IpcMessage_DocumentCodeLensResponse& value) {
  REFLECT_MEMBER_START();
  REFLECT_MEMBER(request_id);
  REFLECT_MEMBER(code_lens);
  REFLECT_MEMBER_END();
}

struct IpcMessage_CodeLensResolveRequest
    : public BaseIpcMessage<IpcMessage_CodeLensResolveRequest> {
  static constexpr IpcId kIpcId = IpcId::CodeLensResolveRequest;
  RequestId request_id;
  TCodeLens code_lens;
};
template <typename TVisitor>
void Reflect(TVisitor& visitor, IpcMessage_CodeLensResolveRequest& value) {
  REFLECT_MEMBER_START();
  REFLECT_MEMBER(request_id);
  REFLECT_MEMBER(code_lens);
  REFLECT_MEMBER_END();
}

struct IpcMessage_CodeLensResolveResponse
    : public BaseIpcMessage<IpcMessage_CodeLensResolveResponse> {
  static constexpr IpcId kIpcId = IpcId::CodeLensResolveResponse;
  RequestId request_id;
  TCodeLens code_lens;
};
template <typename TVisitor>
void Reflect(TVisitor& visitor, IpcMessage_CodeLensResolveResponse& value) {
  REFLECT_MEMBER_START();
  REFLECT_MEMBER(request_id);
  REFLECT_MEMBER(code_lens);
  REFLECT_MEMBER_END();
}

struct IpcMessage_WorkspaceSymbolsResponse
    : public BaseIpcMessage<IpcMessage_WorkspaceSymbolsResponse> {
  static constexpr IpcId kIpcId = IpcId::WorkspaceSymbolsResponse;
  RequestId request_id;
  std::vector<lsSymbolInformation> symbols;
};
template <typename TVisitor>
void Reflect(TVisitor& visitor, IpcMessage_WorkspaceSymbolsResponse& value) {
  REFLECT_MEMBER_START();
  REFLECT_MEMBER(request_id);
  REFLECT_MEMBER(symbols);
  REFLECT_MEMBER_END();
}

struct Timer {
  using Clock = std::chrono::high_resolution_clock;
  std::chrono::time_point<Clock> start_;

  Timer() { Reset(); }

  void Reset() { start_ = Clock::now(); }

  long long ElapsedMilliseconds() {
    std::chrono::time_point<Clock> end = Clock::now();
    return std::chrono::duration_cast<std::chrono::milliseconds>(end - start_)
        .count();
  }
};

void IndexMainLoop(IpcClient* client) {
  std::vector<std::unique_ptr<IpcMessage>> messages = client->TakeMessages();
  for (auto& message : messages) {
    // std::cerr << "Processing message " << static_cast<int>(message->ipc_id)
    // << std::endl;

    switch (message->ipc_id) {
      case IpcId::Quit: {
        exit(0);
        break;
      }

      case IpcId::IndexTranslationUnitRequest: {
        IpcMessage_IndexTranslationUnitRequest* msg =
            static_cast<IpcMessage_IndexTranslationUnitRequest*>(message.get());

        std::cerr << "Parsing file " << msg->path << " with args "
                  << Join(msg->args, ", ") << std::endl;

        Timer time;
        IndexedFile file = Parse(msg->path, msg->args);
        std::cerr << "Parsing/indexing took " << time.ElapsedMilliseconds()
                  << "ms" << std::endl;

        time.Reset();
        IndexUpdate update(file);
        auto response = IpcMessage_IndexTranslationUnitResponse(update);
        std::cerr << "Creating index update took " << time.ElapsedMilliseconds()
                  << "ms" << std::endl;

        time.Reset();
        client->SendToServer(&response);
        std::cerr << "Sending to server took " << time.ElapsedMilliseconds()
                  << "ms" << std::endl;

        break;
      }
    }
  }
}

void IndexMain(int id) {
  IpcClient client_ipc(kIpcIndexerName, id);
  while (true) {
    IndexMainLoop(&client_ipc);
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }
}

QueryableFile* FindFile(QueryableDatabase* db, const std::string& filename) {
  // std::cerr << "Wanted file " << msg->document << std::endl;
  // TODO: hashmap lookup.
  for (auto& file : db->files) {
    // std::cerr << " - Have file " << file.file_id << std::endl;
    if (file.file_id == filename) {
      std::cerr << "Found file " << filename << std::endl;
      return &file;
    }
  }

  std::cerr << "Unable to find file " << filename << std::endl;
  return nullptr;
}

lsLocation GetLsLocation(const QueryableLocation& location) {
  return lsLocation(
      lsDocumentUri::FromPath(location.path),
      lsRange(lsPosition(location.line - 1, location.column - 1)));
}

void AddCodeLens(std::vector<TCodeLens>* result,
                 QueryableLocation loc,
                 const std::vector<QueryableLocation>& uses,
                 bool only_interesting,
                 const char* singular,
                 const char* plural) {
  TCodeLens code_lens;
  code_lens.range.start.line =
      loc.line - 1;  // TODO: cleanup indexer to negate by 1.
  code_lens.range.start.character =
      loc.column - 1;  // TODO: cleanup indexer to negate by 1.
                       // TODO: store range information.
  code_lens.range.end.line = code_lens.range.start.line;
  code_lens.range.end.character = code_lens.range.start.character;

  code_lens.command = lsCommand<lsCodeLensCommandArguments>();
  code_lens.command->command = "superindex.showReferences";
  code_lens.command->arguments.uri = lsDocumentUri::FromPath(loc.path);
  code_lens.command->arguments.position = code_lens.range.start;

  // Add unique uses.
  std::unordered_set<lsLocation> unique_uses;
  for (const QueryableLocation& use : uses) {
    if (only_interesting && !use.interesting)
      continue;
    unique_uses.insert(GetLsLocation(use));
  }
  code_lens.command->arguments.locations.assign(unique_uses.begin(),
                                                unique_uses.end());

  // User visible label
  int num_usages = unique_uses.size();
  code_lens.command->title = std::to_string(num_usages) + " ";
  if (num_usages == 1)
    code_lens.command->title += singular;
  else
    code_lens.command->title += plural;

  if (unique_uses.size() > 0)
    result->push_back(code_lens);
}

void AddCodeLens(std::vector<TCodeLens>* result,
                 QueryableLocation loc,
                 const std::vector<UsrRef>& uses,
                 bool only_interesting,
                 const char* singular,
                 const char* plural) {
  std::vector<QueryableLocation> uses0;
  uses0.reserve(uses.size());
  for (const UsrRef& use : uses)
    uses0.push_back(use.loc);
  AddCodeLens(result, loc, uses0, only_interesting, singular, plural);
}

void AddCodeLens(std::vector<TCodeLens>* result,
                 QueryableDatabase* db,
                 QueryableLocation loc,
                 const std::vector<Usr>& usrs,
                 bool only_interesting,
                 const char* singular,
                 const char* plural) {
  std::vector<QueryableLocation> uses0;
  uses0.reserve(usrs.size());
  for (const Usr& usr : usrs) {
    SymbolIdx symbol = db->usr_to_symbol[usr];
    switch (symbol.kind) {
      case SymbolKind::Type: {
        QueryableTypeDef* def = &db->types[symbol.idx];
        if (def->def.definition)
          uses0.push_back(def->def.definition.value());
        break;
      }
      case SymbolKind::Func: {
        QueryableFuncDef* def = &db->funcs[symbol.idx];
        if (def->def.definition)
          uses0.push_back(def->def.definition.value());
        break;
      }
      case SymbolKind::Var: {
        QueryableVarDef* def = &db->vars[symbol.idx];
        if (def->def.definition)
          uses0.push_back(def->def.definition.value());
        break;
      }
      case SymbolKind::File:
      case SymbolKind::Invalid: {
        assert(false && "unexpected");
        break;
      }
    }
  }
  AddCodeLens(result, loc, uses0, only_interesting, singular, plural);
}

void QueryDbMainLoop(IpcServer* language_client,
                     IpcServer* indexers,
                     QueryableDatabase* db) {
  std::vector<std::unique_ptr<IpcMessage>> messages =
      language_client->TakeMessages();
  for (auto& message : messages) {
    // std::cerr << "Processing message " << static_cast<int>(message->ipc_id)
    // << std::endl;

    switch (message->ipc_id) {
      case IpcId::Quit: {
        break;
      }

      case IpcId::IsAlive: {
        IpcMessage_IsAlive response;
        language_client->SendToClient(0, &response);  // todo: make non-blocking
        break;
      }

      case IpcId::OpenProject: {
        IpcMessage_OpenProject* msg =
            static_cast<IpcMessage_OpenProject*>(message.get());
        std::string path = msg->project_path;

        std::vector<CompilationEntry> entries =
            LoadCompilationEntriesFromDirectory(path);
        for (int i = 0; i < entries.size(); ++i) {
          const CompilationEntry& entry = entries[i];
          std::string filepath = path + "/" + entry.filename;
          std::cerr << "[" << i << "/" << (entries.size() - 1)
                    << "] Dispatching index request for file " << filepath
                    << std::endl;

          // TODO: indexers should steal work and load balance.
          IpcMessage_IndexTranslationUnitRequest request;
          request.path = filepath;
          request.args = entry.args;
          indexers->SendToClient(i % indexers->num_clients(), &request);
          // IndexedFile file = Parse(filepath, entry.args);
          // IndexUpdate update(file);
          // db->ApplyIndexUpdate(&update);
        }
        std::cerr << "Done" << std::endl;
        break;
      }

      case IpcId::DocumentSymbolsRequest: {
        auto msg =
            static_cast<IpcMessage_DocumentSymbolsRequest*>(message.get());

        IpcMessage_DocumentSymbolsResponse response;
        response.request_id = msg->request_id;

        QueryableFile* file = FindFile(db, msg->document);
        if (file) {
          for (UsrRef ref : file->outline) {
            SymbolIdx symbol = db->usr_to_symbol[ref.usr];

            lsSymbolInformation info;
            info.location.range.start.line =
                ref.loc.line - 1;  // TODO: cleanup indexer to negate by 1.
            info.location.range.start.character =
                ref.loc.column - 1;  // TODO: cleanup indexer to negate by 1.
                                     // TODO: store range information.
            info.location.range.end.line = info.location.range.start.line;
            info.location.range.end.character =
                info.location.range.start.character;

            // TODO: cleanup namespace/naming so there is only one SymbolKind.
            switch (symbol.kind) {
              case SymbolKind::Type: {
                QueryableTypeDef& def = db->types[symbol.idx];
                info.name = def.def.qualified_name;
                info.kind = lsSymbolKind::Class;
                break;
              }
              case SymbolKind::Func: {
                QueryableFuncDef& def = db->funcs[symbol.idx];
                info.name = def.def.qualified_name;
                if (def.def.declaring_type.has_value()) {
                  info.kind = lsSymbolKind::Method;
                  Usr declaring = def.def.declaring_type.value();
                  info.containerName =
                      db->types[db->usr_to_symbol[declaring].idx]
                          .def.qualified_name;
                } else {
                  info.kind = lsSymbolKind::Function;
                }
                break;
              }
              case SymbolKind::Var: {
                QueryableVarDef& def = db->vars[symbol.idx];
                info.name = def.def.qualified_name;
                info.kind = lsSymbolKind::Variable;
                break;
              }
              case SymbolKind::File:
              case SymbolKind::Invalid: {
                assert(false && "unexpected");
                break;
              }
            };

            response.symbols.push_back(info);
          }
        }

        language_client->SendToClient(0, &response);
        break;
      }

      case IpcId::DocumentCodeLensRequest: {
        auto msg =
            static_cast<IpcMessage_DocumentCodeLensRequest*>(message.get());

        IpcMessage_DocumentCodeLensResponse response;
        response.request_id = msg->request_id;

        lsDocumentUri file_as_uri;
        file_as_uri.SetPath(msg->document);

        QueryableFile* file = FindFile(db, msg->document);
        if (file) {
          for (UsrRef ref : file->outline) {
            SymbolIdx symbol = db->usr_to_symbol[ref.usr];
            switch (symbol.kind) {
              case SymbolKind::Type: {
                QueryableTypeDef& def = db->types[symbol.idx];
                AddCodeLens(&response.code_lens, ref.loc, def.uses,
                            true /*only_interesting*/, "reference",
                            "references");
                AddCodeLens(&response.code_lens, db, ref.loc, def.derived,
                            false /*only_interesting*/, "derived", "derived");
                break;
              }
              case SymbolKind::Func: {
                QueryableFuncDef& def = db->funcs[symbol.idx];
                AddCodeLens(&response.code_lens, ref.loc, def.uses,
                            false /*only_interesting*/, "reference",
                            "references");
                AddCodeLens(&response.code_lens, ref.loc, def.callers,
                            false /*only_interesting*/, "caller", "callers");
                AddCodeLens(&response.code_lens, ref.loc, def.def.callees,
                            false /*only_interesting*/, "callee", "callees");
                AddCodeLens(&response.code_lens, db, ref.loc, def.derived,
                            false /*only_interesting*/, "derived", "derived");
                break;
              }
              case SymbolKind::Var: {
                QueryableVarDef& def = db->vars[symbol.idx];
                AddCodeLens(&response.code_lens, ref.loc, def.uses,
                            false /*only_interesting*/, "reference",
                            "references");
                break;
              }
              case SymbolKind::File:
              case SymbolKind::Invalid: {
                assert(false && "unexpected");
                break;
              }
            };
          }
        }

        language_client->SendToClient(0, &response);
        break;
      }

      case IpcId::WorkspaceSymbolsRequest: {
        auto msg =
            static_cast<IpcMessage_WorkspaceSymbolsRequest*>(message.get());

        IpcMessage_WorkspaceSymbolsResponse response;
        response.request_id = msg->request_id;

        std::cerr << "- Considering " << db->qualified_names.size()
                  << " candidates " << std::endl;

        for (int i = 0; i < db->qualified_names.size(); ++i) {
          const std::string& name = db->qualified_names[i];
          // std::cerr << "- Considering " << name << std::endl;

          if (name.find(msg->query) != std::string::npos) {
            lsSymbolInformation info;
            info.name = name;

            SymbolIdx symbol = db->symbols[i];

            // TODO: dedup this code w/ above (ie, add ctor to convert symbol to
            // SymbolInformation)
            switch (symbol.kind) {
              // TODO: file
              case SymbolKind::Type: {
                QueryableTypeDef& def = db->types[symbol.idx];
                info.name = def.def.qualified_name;
                info.kind = lsSymbolKind::Class;

                if (def.def.definition.has_value()) {
                  info.location.uri.SetPath(def.def.definition->path);
                  info.location.range.start.line = def.def.definition->line - 1;
                  info.location.range.start.character =
                      def.def.definition->column - 1;
                }
                break;
              }
              case SymbolKind::Func: {
                QueryableFuncDef& def = db->funcs[symbol.idx];
                info.name = def.def.qualified_name;
                if (def.def.declaring_type.has_value()) {
                  info.kind = lsSymbolKind::Method;
                  Usr declaring = def.def.declaring_type.value();
                  info.containerName =
                      db->types[db->usr_to_symbol[declaring].idx]
                          .def.qualified_name;
                } else {
                  info.kind = lsSymbolKind::Function;
                }

                if (def.def.definition.has_value()) {
                  info.location.uri.SetPath(def.def.definition->path);
                  info.location.range.start.line = def.def.definition->line - 1;
                  info.location.range.start.character =
                      def.def.definition->column - 1;
                }
                break;
              }
              case SymbolKind::Var: {
                QueryableVarDef& def = db->vars[symbol.idx];
                info.name = def.def.qualified_name;
                info.kind = lsSymbolKind::Variable;

                if (def.def.definition.has_value()) {
                  info.location.uri.SetPath(def.def.definition->path);
                  info.location.range.start.line = def.def.definition->line - 1;
                  info.location.range.start.character =
                      def.def.definition->column - 1;
                }
                break;
              }
              case SymbolKind::Invalid: {
                assert(false && "unexpected");
                break;
              }
            };

            // TODO: store range information.
            info.location.range.end.line = info.location.range.start.line;
            info.location.range.end.character =
                info.location.range.start.character;

            response.symbols.push_back(info);
          }
        }

        language_client->SendToClient(0, &response);
        break;
      }

      default: {
        std::cerr << "Unhandled IPC message with kind "
                  << static_cast<int>(message->ipc_id) << std::endl;
        exit(1);
      }
    }
  }

  messages = indexers->TakeMessages();
  for (auto& message : messages) {
    // std::cerr << "Processing message " << static_cast<int>(message->ipc_id)
    // << std::endl;
    switch (message->ipc_id) {
      case IpcId::IndexTranslationUnitResponse: {
        IpcMessage_IndexTranslationUnitResponse* msg =
            static_cast<IpcMessage_IndexTranslationUnitResponse*>(
                message.get());
        Timer time;
        db->ApplyIndexUpdate(&msg->update);
        std::cerr << "Applying index update took " << time.ElapsedMilliseconds()
                  << "ms" << std::endl;
        break;
      }

      default: {
        std::cerr << "Unhandled IPC message with kind "
                  << static_cast<int>(message->ipc_id) << std::endl;
        exit(1);
      }
    }
  }
}

void QueryDbMain() {
  std::cerr << "Running QueryDb" << std::endl;
  IpcServer language_client(kIpcLanguageClientName, 1 /*num_clients*/);
  IpcServer indexers(kIpcIndexerName, kNumIndexers);
  QueryableDatabase db;

  std::cerr << "!! starting processes" << std::endl;
  // Start indexer processes.
  for (int i = 0; i < kNumIndexers; ++i) {
    // new Process(process_name + " --indexer " + std::to_string(i + 1));
    new std::thread([i]() { IndexMain(i); });
  }
  std::cerr << "!! done processes" << std::endl;

  // Pump query db main loop.
  while (true) {
    QueryDbMainLoop(&language_client, &indexers, &db);
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }
}

// TODO: global lock on stderr output.

// Separate thread whose only job is to read from stdin and
// dispatch read commands to the actual indexer program. This
// cannot be done on the main thread because reading from std::cin
// blocks.
//
// |ipc| is connected to a server.
void LanguageServerStdinLoop(IpcClient* server) {
  while (true) {
    std::unique_ptr<InMessage> message = ParseMessage();

    // Message parsing can fail if we don't recognize the method.
    if (!message)
      continue;

    std::cerr << "[info]: Got message of type "
              << MethodIdToString(message->method_id) << std::endl;
    switch (message->method_id) {
      case lsMethodId::Initialize: {
        auto request = static_cast<In_InitializeRequest*>(message.get());
        if (request->params.rootUri) {
          std::string project_path = request->params.rootUri->GetPath();
          std::cerr << "Initialize in directory " << project_path
                    << " with uri " << request->params.rootUri->raw_uri
                    << std::endl;
          IpcMessage_OpenProject open_project;
          open_project.project_path = project_path;
          server->SendToServer(&open_project);
        }

        auto response = Out_InitializeResponse();
        response.id = message->id.value();
        response.result.capabilities.documentSymbolProvider = true;
        // response.result.capabilities.referencesProvider = true;
        response.result.capabilities.codeLensProvider = lsCodeLensOptions();
        response.result.capabilities.codeLensProvider->resolveProvider = false;
        response.result.capabilities.workspaceSymbolProvider = true;
        response.Send();
        break;
      }

      case lsMethodId::TextDocumentDocumentSymbol: {
        // TODO: response should take id as input.
        // TODO: message should not have top-level id.
        auto request = static_cast<In_DocumentSymbolRequest*>(message.get());

        IpcMessage_DocumentSymbolsRequest ipc_request;
        ipc_request.request_id = request->id.value();
        ipc_request.document = request->params.textDocument.uri.GetPath();
        std::cerr << "Request textDocument=" << ipc_request.document
                  << std::endl;
        server->SendToServer(&ipc_request);
        break;
      }

      case lsMethodId::TextDocumentCodeLens: {
        auto request = static_cast<In_DocumentCodeLensRequest*>(message.get());

        IpcMessage_DocumentCodeLensRequest ipc_request;
        ipc_request.request_id = request->id.value();
        ipc_request.document = request->params.textDocument.uri.GetPath();
        std::cerr << "Request codeLens on textDocument=" << ipc_request.document
                  << std::endl;
        server->SendToServer(&ipc_request);
        break;
      }

      case lsMethodId::WorkspaceSymbol: {
        auto request = static_cast<In_WorkspaceSymbolRequest*>(message.get());
        IpcMessage_WorkspaceSymbolsRequest ipc_request;
        ipc_request.request_id = request->id.value();
        ipc_request.query = request->params.query;
        std::cerr << "Request query=" << ipc_request.query << std::endl;
        server->SendToServer(&ipc_request);
        break;
      }
    }
  }
}

void LanguageServerMainLoop(IpcClient* ipc) {
  std::vector<std::unique_ptr<IpcMessage>> messages = ipc->TakeMessages();
  for (auto& message : messages) {
    switch (message->ipc_id) {
      case IpcId::Quit: {
        exit(0);
        break;
      }

      case IpcId::DocumentSymbolsResponse: {
        auto msg =
            static_cast<IpcMessage_DocumentSymbolsResponse*>(message.get());

        auto response = Out_DocumentSymbolResponse();
        response.id = msg->request_id;
        response.result = msg->symbols;
        response.Send();
        std::cerr << "Sent symbol response to client ("
                  << response.result.size() << " symbols)" << std::endl;
        break;
      }

      case IpcId::DocumentCodeLensResponse: {
        auto msg =
            static_cast<IpcMessage_DocumentCodeLensResponse*>(message.get());

        auto response = Out_DocumentCodeLensResponse();
        response.id = msg->request_id;
        response.result = msg->code_lens;
        response.Send();
        std::cerr << "Sent code lens response to client ("
                  << response.result.size() << " symbols)" << std::endl;
        break;
      }

      case IpcId::WorkspaceSymbolsResponse: {
        auto msg =
            static_cast<IpcMessage_WorkspaceSymbolsResponse*>(message.get());

        auto response = Out_WorkspaceSymbolResponse();
        response.id = msg->request_id;
        response.result = msg->symbols;
        response.Send();
        std::cerr << "Send symbol response to client ("
                  << response.result.size() << " symbols)" << std::endl;
        break;
      }

      default: {
        std::cerr << "Unhandled IPC message with kind "
                  << static_cast<int>(message->ipc_id) << std::endl;
        exit(1);
      }
    }
  }
}

void LanguageServerMain(std::string process_name) {
  IpcClient client_ipc(kIpcLanguageClientName, 0);

  // Discard any left-over messages from previous runs.
  client_ipc.TakeMessages();

  // Emit an alive check. Sleep so the server has time to respond.
  IpcMessage_IsAlive check_alive;
  client_ipc.SendToServer(&check_alive);

  // TODO: Tune this value or make it configurable.
  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  // Check if we got an IsAlive message back.
  std::vector<std::unique_ptr<IpcMessage>> messages = client_ipc.TakeMessages();
  bool has_server = false;
  for (auto& message : messages) {
    if (IpcId::IsAlive == message->ipc_id) {
      has_server = true;
      break;
    }
  }

// No server is running. Start it.
#if false
  if (!has_server) {
    if (process_name.empty())
      return;

    Process p(process_name + " --querydb", "",
      /*stdout*/[](const char* bytes, size_t n) {
      for (int i = 0; i < n; ++i)
        std::cerr << bytes[i];
    },
      /*stderr*/[](const char* bytes, size_t n) {
      for (int i = 0; i < n; ++i)
        std::cerr << bytes[i];
    },
      /*open_stdin*/false);
    std::this_thread::sleep_for(std::chrono::seconds(1));
    // Pass empty process name so we only try to start the querydb once.
    LanguageServerMain("");
    return;
  }
#endif

  // for debugging attach
  // std::this_thread::sleep_for(std::chrono::seconds(4));

  if (!has_server) {
    // No server. Run it in-process.
    new std::thread([&]() { QueryDbMain(); });
  }

  // Run language client.
  std::thread stdio_reader(&LanguageServerStdinLoop, &client_ipc);
  while (true) {
    LanguageServerMainLoop(&client_ipc);
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }
}

void PreMain() {
// We need to write to stdout in binary mode because in Windows, writing
// \n will implicitly write \r\n. Language server API will ignore a
// \r\r\n split request.
#ifdef _WIN32
  _setmode(_fileno(stdout), O_BINARY);
  _setmode(_fileno(stdin), O_BINARY);
#endif

  IpcRegistry::instance()->Register<IpcMessage_Quit>(IpcMessage_Quit::kIpcId);
  IpcRegistry::instance()->Register<IpcMessage_IsAlive>(
      IpcMessage_IsAlive::kIpcId);
  IpcRegistry::instance()->Register<IpcMessage_OpenProject>(
      IpcMessage_OpenProject::kIpcId);

  IpcRegistry::instance()->Register<IpcMessage_IndexTranslationUnitRequest>(
      IpcMessage_IndexTranslationUnitRequest::kIpcId);
  IpcRegistry::instance()->Register<IpcMessage_IndexTranslationUnitResponse>(
      IpcMessage_IndexTranslationUnitResponse::kIpcId);

  IpcRegistry::instance()->Register<IpcMessage_DocumentSymbolsRequest>(
      IpcMessage_DocumentSymbolsRequest::kIpcId);
  IpcRegistry::instance()->Register<IpcMessage_DocumentSymbolsResponse>(
      IpcMessage_DocumentSymbolsResponse::kIpcId);
  IpcRegistry::instance()->Register<IpcMessage_DocumentCodeLensRequest>(
      IpcMessage_DocumentCodeLensRequest::kIpcId);
  IpcRegistry::instance()->Register<IpcMessage_DocumentCodeLensResponse>(
      IpcMessage_DocumentCodeLensResponse::kIpcId);
  IpcRegistry::instance()->Register<IpcMessage_CodeLensResolveRequest>(
      IpcMessage_CodeLensResolveRequest::kIpcId);
  IpcRegistry::instance()->Register<IpcMessage_CodeLensResolveResponse>(
      IpcMessage_CodeLensResolveResponse::kIpcId);

  IpcRegistry::instance()->Register<IpcMessage_WorkspaceSymbolsRequest>(
      IpcMessage_WorkspaceSymbolsRequest::kIpcId);
  IpcRegistry::instance()->Register<IpcMessage_WorkspaceSymbolsResponse>(
      IpcMessage_WorkspaceSymbolsResponse::kIpcId);
}

template<typename T>
void RegisterId(TypedBidiMessageQueue<lsMethodId, lsBaseMessage>& t) {
  t.RegisterId(T::kMethod,
    [](Writer& visitor, lsBaseMessage& message) {
      T& m = static_cast<T&>(message);
      Reflect(visitor, m);
    }, [](Reader& visitor) {
      auto m = MakeUnique<T>();
      Reflect(visitor, *m);
      return m;
    });
}

int main(int argc, char** argv) {
  // TODO: real queue size
  const int kQueueSize = 128;
  TypedBidiMessageQueue<lsMethodId, lsBaseMessage> t("foo", kQueueSize);
  RegisterId<In_CancelRequest>(t);
  RegisterId<In_InitializeRequest>(t);
  RegisterId<In_InitializedNotification>(t);
  RegisterId<In_DocumentSymbolRequest>(t);
  RegisterId<In_DocumentCodeLensRequest>(t);
  RegisterId<In_WorkspaceSymbolRequest>(t);

  /*
  // TODO: We can make this entire function a template.
  t.RegisterId(In_DocumentSymbolRequest::kMethod,
    [](Writer& visitor, lsBaseMessage& message) {
      In_DocumentSymbolRequest& m = static_cast<In_DocumentSymbolRequest&>(message);
      Reflect(visitor, m);
    }, [](Reader& visitor) {
      auto m = MakeUnique<In_DocumentSymbolRequest>();
      Reflect(visitor, *m);
      return m;
    });
  */

  //struct In_DocumentSymbolRequest : public InRequestMessage {
  //  const static lsMethodId kMethod = lsMethodId::TextDocumentDocumentSymbol;

  //MyMessageType mm;
  //t.SendMessage(&t.for_client, lsMethodId::Initialize, &mm);
  //t.GetMessages(&t.for_client);

  bool loop = false;
  while (loop)
    std::this_thread::sleep_for(std::chrono::milliseconds(10));

  PreMain();

  // if (argc == 1) {
  //  QueryDbMain();
  //  return 0;
  //}
  if (argc == 1) {
    doctest::Context context;
    context.applyCommandLine(argc, argv);
    int res = context.run();
    if (context.shouldExit())
        return res;

    //RunTests();
    return 0;
  }

  std::unordered_map<std::string, std::string> options =
      ParseOptions(argc, argv);

  if (HasOption(options, "--language-server")) {
    std::cerr << "Running language server" << std::endl;
    LanguageServerMain(argv[0]);
    return 0;
  }
  /* TODO: out of process querydb -- maybe?
  else if (HasOption(options, "--querydb")) {
    std::cerr << "Running querydb" << std::endl;
    QueryableDatabase db;
    IpcServer ipc(kIpcServername);
    while (true) {
      QueryDbMainLoop(&ipc, &db);
      // TODO: use a condition variable.
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    return 0;
  }
  */
  else if (HasOption(options, "--indexer")) {
    int index = atoi(options["--indexer"].c_str());
    if (index == 0)
      std::cerr << "--indexer expects an indexer id > 0" << std::endl;
    IndexMain(index);
  } else {
    std::cerr << "Running language server" << std::endl;
    LanguageServerMain(argv[0]);
    return 0;
  }

  return 1;
}
