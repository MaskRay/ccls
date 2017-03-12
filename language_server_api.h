#pragma once

#include <iostream>
#include <unordered_map>
#include <unordered_set>

#include <rapidjson/writer.h>
#include "optional.h"
#include "serializer.h"

using std::experimental::optional;
using std::experimental::nullopt;

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
///////////////////////////// OUTGOING MESSAGES /////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
enum class lsMethodId : int {
  // Language server specific requests.
  CancelRequest = 0,
  Initialize,
  Initialized,
  TextDocumentDocumentSymbol,
  WorkspaceSymbol,
};

template<typename TVisitor>
void Reflect(TVisitor& visitor, lsMethodId& value) {
  int value0 = static_cast<int>(value);
  Reflect(visitor, value0);
  value = static_cast<lsMethodId>(value0);
}

struct RequestId {
  optional<int> id0;
  optional<std::string> id1;
};

void Reflect(Writer& visitor, RequestId& value) {
  assert(value.id0.has_value() || value.id1.has_value());

  if (value.id0) {
    Reflect(visitor, value.id0.value());
  }
  else {
    Reflect(visitor, value.id1.value());
  }
}

void Reflect(Reader& visitor, RequestId& id) {
  if (visitor.IsInt())
    Reflect(visitor, id.id0);
  else if (visitor.IsString())
    Reflect(visitor, id.id1);
  else
    std::cerr << "Unable to deserialize id" << std::endl;
}

struct OutMessage {
  // Write out the body of the message. The writer expects object key/value
  // pairs.
  virtual void WriteMessageBody(Writer& writer) = 0;

  // Send the message to the language client by writing it to stdout.
  void Send() {
    rapidjson::StringBuffer output;
    Writer writer(output);
    writer.StartObject();
    writer.Key("jsonrpc");
    writer.String("2.0");
    WriteMessageBody(writer);
    writer.EndObject();

    std::cout << "Content-Length: " << output.GetSize();
    std::cout << (char)13 << char(10) << char(13) << char(10);
    std::cout << output.GetString();
    std::cout.flush();
  }
};

struct OutRequestMessage : public OutMessage {
  RequestId id;

  virtual std::string Method() = 0;
  virtual void SerializeParams(Writer& visitor) = 0;

  // Message:
  void WriteMessageBody(Writer& visitor) override {
    auto& value = *this;
    auto method = Method();

    REFLECT_MEMBER(id);
    REFLECT_MEMBER2("method", method);

    visitor.Key("params");
    SerializeParams(visitor);
  }
};


struct lsResponseError {
  struct Data {
    virtual void Write(Writer& writer) = 0;
  };

  enum class lsErrorCodes : int {
    ParseError = -32700,
    InvalidRequest = -32600,
    MethodNotFound = -32601,
    InvalidParams = -32602,
    InternalError = -32603,
    serverErrorStart = -32099,
    serverErrorEnd = -32000,
    ServerNotInitialized = -32002,
    UnknownErrorCode = -32001
  };

  lsErrorCodes code;
  // Short description.
  std::string message;
  std::unique_ptr<Data> data;

  void Write(Writer& visitor) {
    auto& value = *this;
    int code = static_cast<int>(this->code);

    visitor.StartObject();
    REFLECT_MEMBER2("code", code);
    REFLECT_MEMBER(message);
    if (data) {
      visitor.Key("data");
      data->Write(visitor);
    }
    visitor.EndObject();
  }
};

struct OutResponseMessage : public OutMessage {
  RequestId id;

  virtual optional<lsResponseError> Error() {
    return nullopt;
  }
  virtual void WriteResult(Writer& visitor) = 0;

  // Message:
  void WriteMessageBody(Writer& visitor) override {
    auto& value = *this;

    REFLECT_MEMBER(id);

    optional<lsResponseError> error = Error();
    if (error) {
      visitor.Key("error");
      error->Write(visitor);
    }
    else {
      visitor.Key("result");
      WriteResult(visitor);
    }
  }
};

struct OutNotificationMessage : public OutMessage {
  virtual std::string Method() = 0;
  virtual void SerializeParams(Writer& writer) = 0;

  // Message:
  void WriteMessageBody(Writer& visitor) override {
    visitor.Key("method");
    std::string method = Method();
    ::Reflect(visitor, method);

    visitor.Key("params");
    SerializeParams(visitor);
  }
};












































/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
///////////////////////////// INCOMING MESSAGES /////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

const char* MethodIdToString(lsMethodId id) {
  switch (id) {
  case lsMethodId::CancelRequest:
    return "$/cancelRequest";
  case lsMethodId::Initialize:
    return "initialize";
  case lsMethodId::Initialized:
    return "initialized";
  case lsMethodId::TextDocumentDocumentSymbol:
    return "textDocument/documentSymbol";
  case lsMethodId::WorkspaceSymbol:
    return "workspace/symbol";
  default:
    exit(1);
  }
}

struct InMessage;

struct MessageRegistry {
  static MessageRegistry* instance_;
  static MessageRegistry* instance();

  using Allocator = std::function<std::unique_ptr<InMessage>(optional<RequestId> id, Reader& params)>;
  std::unordered_map<std::string, Allocator> allocators;

  template<typename T>
  void Register() {
    std::string method_name = MethodIdToString(T::kMethod);
    allocators[method_name] = [](optional<RequestId> id, Reader& params) {
      return MakeUnique<T>(id, params);
    };
  }

  std::unique_ptr<InMessage> Parse(Reader& visitor) {
    std::string jsonrpc = visitor["jsonrpc"].GetString();
    if (jsonrpc != "2.0")
      exit(1);

    optional<RequestId> id;
    ReflectMember(visitor, "id", id);

    std::string method;
    ReflectMember(visitor, "method", method);

    if (allocators.find(method) == allocators.end()) {
      std::cerr << "Unable to find registered handler for method \"" << method << "\"" << std::endl;
      return nullptr;
    }

    Allocator& allocator = allocators[method];


    // We run the allocator with actual params object or a null
    // params object if there are no params. Unifying the two ifs is
    // tricky because the second allocator param is a reference.
    if (visitor.FindMember("params") != visitor.MemberEnd()) {
      Reader& params = visitor["params"];
      return allocator(id, params);
    }
    else {
      Reader params;
      params.SetNull();
      return allocator(id, params);
    }
  }
};

MessageRegistry* MessageRegistry::instance_ = nullptr;
MessageRegistry* MessageRegistry::instance() {
  if (!instance_)
    instance_ = new MessageRegistry();

  return instance_;
}


struct InMessage {
  const lsMethodId method_id;
  optional<RequestId> id;

  InMessage(lsMethodId  method_id, optional<RequestId> id, Reader& reader)
    // We verify there are no duplicate hashes inside of MessageRegistry.
    : method_id(method_id), id(id) {}
};

struct InRequestMessage : public InMessage {
  InRequestMessage(lsMethodId  method, optional<RequestId> id, Reader& reader)
    : InMessage(method, id, reader) {}
};

struct InNotificationMessage : public InMessage {
  InNotificationMessage(lsMethodId  method, optional<RequestId> id, Reader& reader)
    : InMessage(method, id, reader) {}
};














struct In_CancelRequest : public InNotificationMessage {
  static const lsMethodId kMethod = lsMethodId::CancelRequest;

  In_CancelRequest(optional<RequestId> id, Reader& reader)
    : InNotificationMessage(kMethod, id, reader) {}
};




































/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
////////////////////////////// PRIMITIVE TYPES //////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

struct lsDocumentUri {
  std::string raw_uri;

  void SetPath(const std::string& path) {
    // file:///c%3A/Users/jacob/Desktop/superindex/indexer/full_tests
    raw_uri = path;

    size_t index = raw_uri.find(":");
    if (index != -1) {
      raw_uri.replace(raw_uri.begin() + index, raw_uri.begin() + index + 1, "%3A");
    }

    raw_uri = "file:///" + raw_uri;
    //std::cerr << "Set uri to " << raw_uri << " from " << path;
  }

  std::string GetPath() {
    // TODO: make this not a hack.
    std::string result = raw_uri;

    size_t index = result.find("%3A");
    if (index != -1) {
      result.replace(result.begin() + index, result.begin() + index + 3, ":");
    }

    index = result.find("file://");
    if (index != -1) {
      result.replace(result.begin() + index, result.begin() + index + 8, "");
    }

    std::replace(result.begin(), result.end(), '\\', '/');
    return result;
  }
};

template<typename TVisitor>
void Reflect(TVisitor& visitor, lsDocumentUri& value) {
  Reflect(visitor, value.raw_uri);
}


struct lsPosition {
  // Note: these are 0-based.
  int line = 0;
  int character = 0;
};

template<typename TVisitor>
void Reflect(TVisitor& visitor, lsPosition& value) {
  REFLECT_MEMBER_START();
  REFLECT_MEMBER(line);
  REFLECT_MEMBER(character);
  REFLECT_MEMBER_END();
}


struct lsRange {
  lsPosition start;
  lsPosition end;
};

template<typename TVisitor>
void Reflect(TVisitor& visitor, lsRange& value) {
  REFLECT_MEMBER_START();
  REFLECT_MEMBER(start);
  REFLECT_MEMBER(end);
  REFLECT_MEMBER_END();
}


struct lsLocation {
  lsDocumentUri uri;
  lsRange range;
};

template<typename TVisitor>
void Reflect(TVisitor& visitor, lsLocation& value) {
  REFLECT_MEMBER_START();
  REFLECT_MEMBER(uri);
  REFLECT_MEMBER(range);
  REFLECT_MEMBER_END();
}


enum class lsSymbolKind : int {
  File = 1,
  Module = 2,
  Namespace = 3,
  Package = 4,
  Class = 5,
  Method = 6,
  Property = 7,
  Field = 8,
  Constructor = 9,
  Enum = 10,
  Interface = 11,
  Function = 12,
  Variable = 13,
  Constant = 14,
  String = 15,
  Number = 16,
  Boolean = 17,
  Array = 18
};

void Reflect(Writer& writer, lsSymbolKind& value) {
  writer.Int(static_cast<int>(value));
}

void Reflect(Reader& reader, lsSymbolKind& value) {
  value = static_cast<lsSymbolKind>(reader.GetInt());
}


struct lsSymbolInformation {
  std::string name;
  lsSymbolKind kind;
  lsLocation location;
  std::string containerName;
};

template<typename TVisitor>
void Reflect(TVisitor& visitor, lsSymbolInformation& value) {
  REFLECT_MEMBER_START();
  REFLECT_MEMBER(name);
  REFLECT_MEMBER(kind);
  REFLECT_MEMBER(location);
  REFLECT_MEMBER(containerName);
  REFLECT_MEMBER_END();
}


struct lsCommand {
  // Title of the command (ie, 'save')
  std::string title;
  // Actual command identifier.
  std::string command;
  // Arguments to run the command with. Could be JSON objects.
  std::vector<std::string> arguments;
};

template<typename TVisitor>
void Reflect(TVisitor& visitor, lsCommand& value) {
  REFLECT_MEMBER_START();
  REFLECT_MEMBER(title);
  REFLECT_MEMBER(command);
  REFLECT_MEMBER(arguments);
  REFLECT_MEMBER_END();
}


// TODO: TextDocumentEdit
// TODO: WorkspaceEdit

struct lsTextDocumentIdentifier {
  lsDocumentUri uri;
};

template<typename TVisitor>
void Reflect(TVisitor& visitor, lsTextDocumentIdentifier& value) {
  REFLECT_MEMBER_START();
  REFLECT_MEMBER(uri);
  REFLECT_MEMBER_END();
}

// TODO: TextDocumentItem
// TODO: VersionedTextDocumentIdentifier
// TODO: TextDocumentPositionParams
// TODO: DocumentFilter 
// TODO: DocumentSelector 

































































































/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
////////////////////////////// INITIALIZATION ///////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

// Workspace specific client capabilities.
struct lsWorkspaceClientCapabilites {
  // The client supports applying batch edits to the workspace.
  optional<bool> applyEdit;

  struct lsWorkspaceEdit {
    // The client supports versioned document changes in `WorkspaceEdit`s
    optional<bool> documentChanges;
  };

  // Capabilities specific to `WorkspaceEdit`s
  optional<lsWorkspaceEdit> workspaceEdit;


  struct lsGenericDynamicReg {
    // Did foo notification supports dynamic registration.
    optional<bool> dynamicRegistration;
  };


  // Capabilities specific to the `workspace/didChangeConfiguration` notification.
  optional<lsGenericDynamicReg> didChangeConfiguration;

  // Capabilities specific to the `workspace/didChangeWatchedFiles` notification.
  optional<lsGenericDynamicReg> didChangeWatchedFiles;

  // Capabilities specific to the `workspace/symbol` request.
  optional<lsGenericDynamicReg> symbol;

  // Capabilities specific to the `workspace/executeCommand` request.
  optional<lsGenericDynamicReg> executeCommand;
};

template<typename TVisitor>
void Reflect(TVisitor& visitor, lsWorkspaceClientCapabilites::lsWorkspaceEdit& value) {
  REFLECT_MEMBER_START();
  REFLECT_MEMBER(documentChanges);
  REFLECT_MEMBER_END();
}

template<typename TVisitor>
void Reflect(TVisitor& visitor, lsWorkspaceClientCapabilites::lsGenericDynamicReg& value) {
  REFLECT_MEMBER_START();
  REFLECT_MEMBER(dynamicRegistration);
  REFLECT_MEMBER_END();
}

template<typename TVisitor>
void Reflect(TVisitor& visitor, lsWorkspaceClientCapabilites& value) {
  REFLECT_MEMBER_START();
  REFLECT_MEMBER(applyEdit);
  REFLECT_MEMBER(workspaceEdit);
  REFLECT_MEMBER(didChangeConfiguration);
  REFLECT_MEMBER(didChangeWatchedFiles);
  REFLECT_MEMBER(symbol);
  REFLECT_MEMBER(executeCommand);
  REFLECT_MEMBER_END();
}


// Text document specific client capabilities.
struct lsTextDocumentClientCapabilities {
  struct lsSynchronization {
    // Whether text document synchronization supports dynamic registration.
    optional<bool> dynamicRegistration;

    // The client supports sending will save notifications.
    optional<bool> willSave;

    // The client supports sending a will save request and
    // waits for a response providing text edits which will
    // be applied to the document before it is saved.
    optional<bool> willSaveWaitUntil;

    // The client supports did save notifications.
    optional<bool> didSave;
  };

  lsSynchronization synchronization;

  struct lsCompletion {
    // Whether completion supports dynamic registration.
    optional<bool> dynamicRegistration;

    struct lsCompletionItem {
      // Client supports snippets as insert text.
      //
      // A snippet can define tab stops and placeholders with `$1`, `$2`
      // and `${3:foo}`. `$0` defines the final tab stop, it defaults to
      // the end of the snippet. Placeholders with equal identifiers are linked,
      // that is typing in one will update others too.
      optional<bool> snippetSupport;
    };

    // The client supports the following `CompletionItem` specific
    // capabilities.
    optional<lsCompletionItem> completionItem;
  };
  // Capabilities specific to the `textDocument/completion`
  optional<lsCompletion> completion;

  struct lsGenericDynamicReg {
    // Whether foo supports dynamic registration.
    optional<bool> dynamicRegistration;
  };

  // Capabilities specific to the `textDocument/hover`
  optional<lsGenericDynamicReg> hover;

  // Capabilities specific to the `textDocument/signatureHelp`
  optional<lsGenericDynamicReg> signatureHelp;

  // Capabilities specific to the `textDocument/references`
  optional<lsGenericDynamicReg> references;

  // Capabilities specific to the `textDocument/documentHighlight`
  optional<lsGenericDynamicReg> documentHighlight;

  // Capabilities specific to the `textDocument/documentSymbol`
  optional<lsGenericDynamicReg> documentSymbol;

  // Capabilities specific to the `textDocument/formatting`
  optional<lsGenericDynamicReg> formatting;

  // Capabilities specific to the `textDocument/rangeFormatting`
  optional<lsGenericDynamicReg> rangeFormatting;

  // Capabilities specific to the `textDocument/onTypeFormatting`
  optional<lsGenericDynamicReg> onTypeFormatting;

  // Capabilities specific to the `textDocument/definition`
  optional<lsGenericDynamicReg> definition;

  // Capabilities specific to the `textDocument/codeAction`
  optional<lsGenericDynamicReg> codeAction;

  // Capabilities specific to the `textDocument/codeLens`
  optional<lsGenericDynamicReg> codeLens;

  // Capabilities specific to the `textDocument/documentLink`
  optional<lsGenericDynamicReg> documentLink;

  // Capabilities specific to the `textDocument/rename`
  optional<lsGenericDynamicReg> rename;
};

template<typename TVisitor>
void Reflect(TVisitor& visitor, lsTextDocumentClientCapabilities::lsSynchronization& value) {
  REFLECT_MEMBER_START();
  REFLECT_MEMBER(dynamicRegistration);
  REFLECT_MEMBER(willSave);
  REFLECT_MEMBER(willSaveWaitUntil);
  REFLECT_MEMBER(didSave);
  REFLECT_MEMBER_END();
}

template<typename TVisitor>
void Reflect(TVisitor& visitor, lsTextDocumentClientCapabilities::lsCompletion& value) {
  REFLECT_MEMBER_START();
  REFLECT_MEMBER(dynamicRegistration);
  REFLECT_MEMBER(completionItem);
  REFLECT_MEMBER_END();
}

template<typename TVisitor>
void Reflect(TVisitor& visitor, lsTextDocumentClientCapabilities::lsCompletion::lsCompletionItem& value) {
  REFLECT_MEMBER_START();
  REFLECT_MEMBER(snippetSupport);
  REFLECT_MEMBER_END();
}

template<typename TVisitor>
void Reflect(TVisitor& visitor, lsTextDocumentClientCapabilities::lsGenericDynamicReg& value) {
  REFLECT_MEMBER_START();
  REFLECT_MEMBER(dynamicRegistration);
  REFLECT_MEMBER_END();
}

template<typename TVisitor>
void Reflect(TVisitor& visitor, lsTextDocumentClientCapabilities& value) {
  REFLECT_MEMBER_START();
  REFLECT_MEMBER(synchronization);
  REFLECT_MEMBER(completion);
  REFLECT_MEMBER(hover);
  REFLECT_MEMBER(signatureHelp);
  REFLECT_MEMBER(references);
  REFLECT_MEMBER(documentHighlight);
  REFLECT_MEMBER(documentSymbol);
  REFLECT_MEMBER(formatting);
  REFLECT_MEMBER(rangeFormatting);
  REFLECT_MEMBER(onTypeFormatting);
  REFLECT_MEMBER(definition);
  REFLECT_MEMBER(codeAction);
  REFLECT_MEMBER(codeLens);
  REFLECT_MEMBER(documentLink);
  REFLECT_MEMBER(rename);
  REFLECT_MEMBER_END();
}

struct lsClientCapabilities {
  // Workspace specific client capabilities.
  optional<lsWorkspaceClientCapabilites> workspace;

  // Text document specific client capabilities.
  optional<lsTextDocumentClientCapabilities> textDocument;

  /**
  * Experimental client capabilities.
  */
  // experimental?: any; // TODO
};

template<typename TVisitor>
void Reflect(TVisitor& visitor, lsClientCapabilities& value) {
  REFLECT_MEMBER_START();
  REFLECT_MEMBER(workspace);
  REFLECT_MEMBER(textDocument);
  REFLECT_MEMBER_END();
}

struct lsInitializeParams {
  // The process Id of the parent process that started
  // the server. Is null if the process has not been started by another process.
  // If the parent process is not alive then the server should exit (see exit notification) its process.
  optional<int> processId;

  // The rootPath of the workspace. Is null
  // if no folder is open.
  //
  // @deprecated in favour of rootUri.
  optional<std::string> rootPath;

  // The rootUri of the workspace. Is null if no
  // folder is open. If both `rootPath` and `rootUri` are set
  // `rootUri` wins.
  optional<lsDocumentUri> rootUri;

  // User provided initialization options.
  // initializationOptions?: any; // TODO

  // The capabilities provided by the client (editor or tool)
  lsClientCapabilities capabilities;

  enum class lsTrace {
    // NOTE: serialized as a string, one of 'off' | 'messages' | 'verbose';
    Off, // off
    Messages, // messages
    Verbose // verbose
  };

  // The initial trace setting. If omitted trace is disabled ('off').
  lsTrace trace = lsTrace::Off;
};

void Reflect(Reader& reader, lsInitializeParams::lsTrace& value) {
  std::string v = reader.GetString();
  if (v == "off")
    value = lsInitializeParams::lsTrace::Off;
  else if (v == "messages")
    value = lsInitializeParams::lsTrace::Messages;
  else if (v == "verbose")
    value = lsInitializeParams::lsTrace::Verbose;
}

template<typename TVisitor>
void Reflect(TVisitor& visitor, lsInitializeParams& value) {
  REFLECT_MEMBER_START();
  REFLECT_MEMBER(processId);
  REFLECT_MEMBER(rootPath);
  REFLECT_MEMBER(rootUri);
  REFLECT_MEMBER(capabilities);
  REFLECT_MEMBER(trace);
  REFLECT_MEMBER_END();
}


#if false
/**
 * Known error codes for an `InitializeError`;
 */
export namespace lsInitializeError {
  /**
   * If the protocol version provided by the client can't be handled by the server.
   * @deprecated This initialize error got replaced by client capabilities. There is
   * no version handshake in version 3.0x
   */
  export const unknownProtocolVersion : number = 1;
}
#endif

struct lsInitializeError {
  // Indicates whether the client should retry to send the
  // initilize request after showing the message provided
  // in the ResponseError.
  bool retry;
};

template<typename TVisitor>
void Reflect(TVisitor& visitor, lsInitializeError& value) {
  REFLECT_MEMBER_START();
  REFLECT_MEMBER(retry);
  REFLECT_MEMBER_END();
}

// Defines how the host (editor) should sync document changes to the language server.
enum class lsTextDocumentSyncKind {
  // Documents should not be synced at all.
  None = 0,

  // Documents are synced by always sending the full content
  // of the document.
  Full = 1,

  // Documents are synced by sending the full content on open.
  // After that only incremental updates to the document are
  // send.
  Incremental = 2
};

void Reflect(Writer& writer, lsTextDocumentSyncKind& value) {
  writer.Int(static_cast<int>(value));
}

// Completion options.
struct lsCompletionOptions {
  // The server provides support to resolve additional
  // information for a completion item.
  bool resolveProvider = false;

  // The characters that trigger completion automatically.
  std::vector<std::string> triggerCharacters;
};

template<typename TVisitor>
void Reflect(TVisitor& visitor, lsCompletionOptions& value) {
  REFLECT_MEMBER_START();
  REFLECT_MEMBER(resolveProvider);
  REFLECT_MEMBER(triggerCharacters);
  REFLECT_MEMBER_END();
}

// Signature help options.
struct lsSignatureHelpOptions {
  // The characters that trigger signature help automatically.
  std::vector<std::string> triggerCharacters;
};

template<typename TVisitor>
void Reflect(TVisitor& visitor, lsSignatureHelpOptions& value) {
  REFLECT_MEMBER_START();
  REFLECT_MEMBER(triggerCharacters);
  REFLECT_MEMBER_END();
}

// Code Lens options.
struct lsCodeLensOptions {
  // Code lens has a resolve provider as well.
  bool resolveProvider = false;
};

template<typename TVisitor>
void Reflect(TVisitor& visitor, lsCodeLensOptions& value) {
  REFLECT_MEMBER_START();
  REFLECT_MEMBER(resolveProvider);
  REFLECT_MEMBER_END();
}

// Format document on type options
struct lsDocumentOnTypeFormattingOptions {
  // A character on which formatting should be triggered, like `}`.
  std::string firstTriggerCharacter;

  // More trigger characters.
  std::vector<std::string> moreTriggerCharacter;
};

template<typename TVisitor>
void Reflect(TVisitor& visitor, lsDocumentOnTypeFormattingOptions& value) {
  REFLECT_MEMBER_START();
  REFLECT_MEMBER(firstTriggerCharacter);
  REFLECT_MEMBER(moreTriggerCharacter);
  REFLECT_MEMBER_END();
}

// Document link options
struct lsDocumentLinkOptions {
  // Document links have a resolve provider as well.
  bool resolveProvider = false;
};

template<typename TVisitor>
void Reflect(TVisitor& visitor, lsDocumentLinkOptions& value) {
  REFLECT_MEMBER_START();
  REFLECT_MEMBER(resolveProvider);
  REFLECT_MEMBER_END();
}

// Execute command options.
struct lsExecuteCommandOptions {
  // The commands to be executed on the server
  std::vector<std::string> commands;
};

template<typename TVisitor>
void Reflect(TVisitor& visitor, lsExecuteCommandOptions& value) {
  REFLECT_MEMBER_START();
  REFLECT_MEMBER(commands);
  REFLECT_MEMBER_END();
}

// Save options.
struct lsSaveOptions {
  // The client is supposed to include the content on save.
  bool includeText = false;
};

template<typename TVisitor>
void Reflect(TVisitor& visitor, lsSaveOptions& value) {
  REFLECT_MEMBER_START();
  REFLECT_MEMBER(includeText);
  REFLECT_MEMBER_END();
}

struct lsTextDocumentSyncOptions {
  // Open and close notifications are sent to the server.
  bool openClose = false;
  // Change notificatins are sent to the server. See TextDocumentSyncKind.None, TextDocumentSyncKind.Full
  // and TextDocumentSyncKindIncremental.
  optional<lsTextDocumentSyncKind> change;
  // Will save notifications are sent to the server.
  bool willSave = false;
  // Will save wait until requests are sent to the server.
  bool willSaveWaitUntil = false;
  // Save notifications are sent to the server.
  optional<lsSaveOptions> save;
};

template<typename TVisitor>
void Reflect(TVisitor& visitor, lsTextDocumentSyncOptions& value) {
  REFLECT_MEMBER_START();
  REFLECT_MEMBER(openClose);
  REFLECT_MEMBER(change);
  REFLECT_MEMBER(willSave);
  REFLECT_MEMBER(willSaveWaitUntil);
  REFLECT_MEMBER(save);
  REFLECT_MEMBER_END();
}

struct lsServerCapabilities {
  // Defines how text documents are synced. Is either a detailed structure defining each notification or
  // for backwards compatibility the TextDocumentSyncKind number.
  optional<lsTextDocumentSyncOptions> textDocumentSync;
  // The server provides hover support.
  bool hoverProvider = false;
  // The server provides completion support.
  optional<lsCompletionOptions> completionProvider;
  // The server provides signature help support.
  optional<lsSignatureHelpOptions> signatureHelpProvider;
  // The server provides goto definition support.
  bool definitionProvider = false;
  // The server provides find references support.
  bool referencesProvider = false;
  // The server provides document highlight support.
  bool documentHighlightProvider = false;
  // The server provides document symbol support.
  bool documentSymbolProvider = false;
  // The server provides workspace symbol support.
  bool workspaceSymbolProvider = false;
  // The server provides code actions.
  bool codeActionProvider = false;
  // The server provides code lens.
  optional<lsCodeLensOptions> codeLensProvider;
  // The server provides document formatting.
  bool documentFormattingProvider = false;
  // The server provides document range formatting.
  bool documentRangeFormattingProvider = false;
  // The server provides document formatting on typing.
  optional<lsDocumentOnTypeFormattingOptions> documentOnTypeFormattingProvider;
  // The server provides rename support.
  bool renameProvider = false;
  // The server provides document link support.
  optional<lsDocumentLinkOptions> documentLinkProvider;
  // The server provides execute command support.
  optional<lsExecuteCommandOptions> executeCommandProvider;
};

template<typename TVisitor>
void Reflect(TVisitor& visitor, lsServerCapabilities& value) {
  REFLECT_MEMBER_START();
  REFLECT_MEMBER(textDocumentSync);
  REFLECT_MEMBER(hoverProvider);
  REFLECT_MEMBER(completionProvider);
  REFLECT_MEMBER(signatureHelpProvider);
  REFLECT_MEMBER(definitionProvider);
  REFLECT_MEMBER(referencesProvider);
  REFLECT_MEMBER(documentHighlightProvider);
  REFLECT_MEMBER(documentSymbolProvider);
  REFLECT_MEMBER(workspaceSymbolProvider);
  REFLECT_MEMBER(codeActionProvider);
  REFLECT_MEMBER(codeLensProvider);
  REFLECT_MEMBER(documentFormattingProvider);
  REFLECT_MEMBER(documentRangeFormattingProvider);
  REFLECT_MEMBER(documentOnTypeFormattingProvider);
  REFLECT_MEMBER(renameProvider);
  REFLECT_MEMBER(documentLinkProvider);
  REFLECT_MEMBER(executeCommandProvider);
  REFLECT_MEMBER_END();
}

struct lsInitializeResult {
  // The capabilities the language server provides.
  lsServerCapabilities capabilities;
};

template<typename TVisitor>
void Reflect(TVisitor& visitor, lsInitializeResult& value) {
  REFLECT_MEMBER_START();
  REFLECT_MEMBER(capabilities);
  REFLECT_MEMBER_END();
}


struct In_InitializeRequest : public InRequestMessage {
  const static lsMethodId kMethod = lsMethodId::Initialize;
  lsInitializeParams params;

  In_InitializeRequest(optional<RequestId> id, Reader& reader)
    : InRequestMessage(kMethod, id, reader) {
    auto type = reader.GetType();
    Reflect(reader, params);
    std::cerr << "done" << std::endl;
  }
};

struct Out_InitializeResponse : public OutResponseMessage {
  lsInitializeResult result;

  // OutResponseMessage:
  void WriteResult(Writer& writer) override {
    Reflect(writer, result);
  }
};

struct In_InitializedNotification : public InNotificationMessage {
  const static lsMethodId kMethod = lsMethodId::Initialized;

  In_InitializedNotification(optional<RequestId> id, Reader& reader)
    : InNotificationMessage(kMethod, id, reader) {}
};














































struct lsDocumentSymbolParams {
  lsTextDocumentIdentifier textDocument;
};

template<typename TVisitor>
void Reflect(TVisitor& visitor, lsDocumentSymbolParams& value) {
  REFLECT_MEMBER_START();
  REFLECT_MEMBER(textDocument);
  REFLECT_MEMBER_END();
}

struct In_DocumentSymbolRequest : public InRequestMessage {
  const static lsMethodId kMethod = lsMethodId::TextDocumentDocumentSymbol;

  lsDocumentSymbolParams params;

  In_DocumentSymbolRequest(optional<RequestId> id, Reader& reader)
    : InRequestMessage(kMethod, id, reader) {
    Reflect(reader, params);
  }
};


struct Out_DocumentSymbolResponse : public OutResponseMessage {
  std::vector<lsSymbolInformation> result;

  // OutResponseMessage:
  void WriteResult(Writer& writer) override {
    ::Reflect(writer, result);
  }
};









struct lsWorkspaceSymbolParams {
  std::string query;
};

template<typename TVisitor>
void Reflect(TVisitor& visitor, lsWorkspaceSymbolParams& value) {
  REFLECT_MEMBER_START();
  REFLECT_MEMBER(query);
  REFLECT_MEMBER_END();
}

struct In_WorkspaceSymbolRequest : public InRequestMessage {
  const static lsMethodId kMethod = lsMethodId::WorkspaceSymbol;

  lsWorkspaceSymbolParams params;

  In_WorkspaceSymbolRequest(optional<RequestId> id, Reader& reader)
    : InRequestMessage(kMethod, id, reader) {
    Reflect(reader, params);
  }
};


struct Out_WorkspaceSymbolResponse : public OutResponseMessage {
  std::vector<lsSymbolInformation> result;

  // OutResponseMessage:
  void WriteResult(Writer& writer) override {
    ::Reflect(writer, result);
  }
};

































enum class lsMessageType : int {
  Error = 1,
  Warning = 2,
  Info = 3,
  Log = 4
};

template<typename TWriter>
void Reflect(TWriter& writer, lsMessageType& value) {
  int value0 = static_cast<int>(value);
  Reflect(writer, value0);
  value = static_cast<lsMessageType>(value0);
}

struct ShowMessageOutNotification : public OutNotificationMessage {
  lsMessageType type = lsMessageType::Error;
  std::string message;

  // OutNotificationMessage:
  std::string Method() override {
    return "window/showMessage";
  }
  void SerializeParams(Writer& visitor) override {
    auto& value = *this;
    REFLECT_MEMBER_START();
    REFLECT_MEMBER(type);
    REFLECT_MEMBER(message);
    REFLECT_MEMBER_END();
  }
};

struct LogMessageOutNotification : public OutNotificationMessage {
  lsMessageType type = lsMessageType::Error;
  std::string message;

  // OutNotificationMessage:
  std::string Method() override {
    return "window/logMessage";
  }
  void SerializeParams(Writer& visitor) override {
    auto& value = *this;
    REFLECT_MEMBER_START();
    REFLECT_MEMBER(type);
    REFLECT_MEMBER(message);
    REFLECT_MEMBER_END();
  }
};
