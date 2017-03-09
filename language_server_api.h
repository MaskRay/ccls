#pragma once

#include <iostream>
#include <unordered_map>
#include <unordered_set>

#include <rapidjson/writer.h>
#include "optional.h"
#include "serializer.h"

using std::experimental::optional;
using std::experimental::nullopt;

namespace language_server_api {

  /////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////
  ///////////////////////////// OUTGOING MESSAGES /////////////////////////////
  /////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////

  struct RequestId {
    optional<int> id0;
    optional<std::string> id1;
  };
}

void Reflect(Writer& visitor, language_server_api::RequestId& value) {
  if (value.id0) {
    Reflect(visitor, value.id0.value());
  }
  else {
    assert(value.id1.has_value());
    Reflect(visitor, value.id1.value());
  }
}

void Reflect(Reader& visitor, language_server_api::RequestId& id) {
  if (visitor.IsInt())
    Reflect(visitor, id.id0);
  else if (visitor.IsString())
    Reflect(visitor, id.id1);
  else
    std::cerr << "Unable to deserialize id" << std::endl;
}

namespace language_server_api {

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

  struct OutRequestMessage : OutMessage {
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


  struct ResponseError {
    struct Data {
      virtual void Write(Writer& writer) = 0;
    };

    enum class ErrorCodes : int {
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

    ErrorCodes code;
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

  struct OutResponseMessage : OutMessage {
    RequestId id;

    virtual optional<ResponseError> Error() {
      return nullopt;
    }
    virtual void WriteResult(Writer& visitor) = 0;

    // Message:
    void WriteMessageBody(Writer& visitor) override {
      auto& value = *this;

      REFLECT_MEMBER(id);

      optional<ResponseError> error = Error();
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

  struct OutNotificationMessage : OutMessage {
    virtual std::string Method() = 0;
    virtual void SerializeParams(Writer& writer) = 0;

    // Message:
    void WriteMessageBody(Writer& visitor) override {
      visitor.Key("method");
      std::string method = Method();
      Reflect(visitor, method);

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

  enum class MethodId {
    CancelRequest,
    Initialize,
    Initialized,
    TextDocumentDocumentSymbol,
    WorkspaceSymbol,
  };

  const char* MethodIdToString(MethodId id) {
    switch (id) {
    case MethodId::CancelRequest:
      return "$/cancelRequest";
    case MethodId::Initialize:
      return "initialize";
    case MethodId::Initialized:
      return "initialized";
    case MethodId::TextDocumentDocumentSymbol:
      return "textDocument/documentSymbol";
    case MethodId::WorkspaceSymbol:
      return "workspace/symbol";
    default:
      exit(1);
    }
  }

  struct InMessage;

  struct MessageRegistry {
    static MessageRegistry* instance_;
    static MessageRegistry* instance();

    using Allocator = std::function<std::unique_ptr<InMessage>(optional<RequestId> id, const Reader& params)>;
    std::unordered_map<std::string, Allocator> allocators;

    template<typename T>
    void Register() {
      std::string method_name = MethodIdToString(T::kMethod);
      allocators[method_name] = [](optional<RequestId> id, const Reader& params) {
        return MakeUnique<T>(id, params);
      };
    }

    std::unique_ptr<InMessage> Parse(const Reader& visitor) {
      std::string jsonrpc = visitor["jsonrpc"].GetString();
      if (jsonrpc != "2.0")
        exit(1);

      optional<RequestId> id;
      if (visitor.FindMember("id") != visitor.MemberEnd())
        ::Deserialize(visitor["id"], id);

      std::string method;
      ::Deserialize(visitor["method"], method);

      if (allocators.find(method) == allocators.end()) {
        std::cerr << "Unable to find registered handler for method \"" << method << "\"" << std::endl;
        return nullptr;
      }

      Allocator& allocator = allocators[method];


      // We run the allocator with actual params object or a null
      // params object if there are no params. Unifying the two ifs is
      // tricky because the second allocator param is a reference.
      if (visitor.FindMember("params") != visitor.MemberEnd()) {
        const Reader& params = visitor["params"];
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
    const MethodId method_id;
    optional<RequestId> id;

    InMessage(MethodId method_id, optional<RequestId> id, const Reader& reader)
      // We verify there are no duplicate hashes inside of MessageRegistry.
      : method_id(method_id), id(id) {}
  };

  struct InRequestMessage : public InMessage {
    InRequestMessage(MethodId method, optional<RequestId> id, const Reader& reader)
      : InMessage(method, id, reader) {}
  };

  struct InNotificationMessage : public InMessage {
    InNotificationMessage(MethodId method, optional<RequestId> id, const Reader& reader)
      : InMessage(method, id, reader) {}
  };














  struct In_CancelRequest : public InNotificationMessage {
    static const MethodId kMethod = MethodId::CancelRequest;

    In_CancelRequest(optional<RequestId> id, const Reader& reader)
      : InNotificationMessage(kMethod, id, reader) {}
  };




































  /////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////
  ////////////////////////////// PRIMITIVE TYPES //////////////////////////////
  /////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////

  // Keep all types in the language_server_api namespace in sync with language server spec.
  // TODO
  struct DocumentUri {
    std::string raw_uri;

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
  void Reflect(TVisitor& visitor, DocumentUri& value) {
    Reflect(visitor, value.raw_uri);
  }


  struct Position {
    // Note: these are 0-based.
    int line = 0;
    int character = 0;
  };

  template<typename TVisitor>
  void Reflect(TVisitor& visitor, Position& value) {
    REFLECT_MEMBER_START();
    REFLECT_MEMBER(line);
    REFLECT_MEMBER(character);
    REFLECT_MEMBER_END();
  }


  struct Range {
    Position start;
    Position end;
  };

  template<typename TVisitor>
  void Reflect(TVisitor& visitor, Range& value) {
    REFLECT_MEMBER_START();
    REFLECT_MEMBER(start);
    REFLECT_MEMBER(end);
    REFLECT_MEMBER_END();
  }


  struct Location {
    DocumentUri uri;
    Range range;
  };

  template<typename TVisitor>
  void Reflect(TVisitor& visitor, Location& value) {
    REFLECT_MEMBER_START();
    REFLECT_MEMBER(uri);
    REFLECT_MEMBER(range);
    REFLECT_MEMBER_END();
  }


  enum class SymbolKind : int {
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

  void Reflect(Writer& writer, SymbolKind& value) {
    writer.Int(static_cast<int>(value));
  }

  void Reflect(Reader& reader, SymbolKind& value) {
    value = static_cast<SymbolKind>(reader.GetInt());
  }


  struct SymbolInformation {
    std::string name;
    SymbolKind kind;
    Location location;
    std::string containerName;
  };

  template<typename TVisitor>
  void Reflect(TVisitor& visitor, SymbolInformation& value) {
    REFLECT_MEMBER_START();
    REFLECT_MEMBER(name);
    REFLECT_MEMBER(kind);
    REFLECT_MEMBER(location);
    REFLECT_MEMBER(containerName);
    REFLECT_MEMBER_END();
  }


  struct Command {
    // Title of the command (ie, 'save')
    std::string title;
    // Actual command identifier.
    std::string command;
    // Arguments to run the command with. Could be JSON objects.
    std::vector<std::string> arguments;
  };

  template<typename TVisitor>
  void Reflect(TVisitor& visitor, Command& value) {
    REFLECT_MEMBER_START();
    REFLECT_MEMBER(title);
    REFLECT_MEMBER(command);
    REFLECT_MEMBER(arguments);
    REFLECT_MEMBER_END();
  }


  // TODO: TextDocumentEdit
  // TODO: WorkspaceEdit

  struct TextDocumentIdentifier {
    DocumentUri uri;
  };

  void Serialize(Writer& writer, const TextDocumentIdentifier& value) {
    writer.StartObject();
    SERIALIZE_MEMBER(uri);
    writer.EndObject();
  }

  void Deserialize(const Reader& reader, TextDocumentIdentifier& value) {
    DESERIALIZE_MEMBER(uri);
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
  struct WorkspaceClientCapabilites {
    // The client supports applying batch edits to the workspace.
    optional<bool> applyEdit;

    struct WorkspaceEdit {
      // The client supports versioned document changes in `WorkspaceEdit`s
      optional<bool> documentChanges;
    };

    // Capabilities specific to `WorkspaceEdit`s
    optional<WorkspaceEdit> workspaceEdit;


    struct GenericDynamicReg {
      // Did foo notification supports dynamic registration.
      optional<bool> dynamicRegistration;
    };


    // Capabilities specific to the `workspace/didChangeConfiguration` notification.
    optional<GenericDynamicReg> didChangeConfiguration;

    // Capabilities specific to the `workspace/didChangeWatchedFiles` notification.
    optional<GenericDynamicReg> didChangeWatchedFiles;

    // Capabilities specific to the `workspace/symbol` request.
    optional<GenericDynamicReg> symbol;

    // Capabilities specific to the `workspace/executeCommand` request.
    optional<GenericDynamicReg> executeCommand;
  };

  template<typename TVisitor>
  void Reflect(TVisitor& visitor, WorkspaceClientCapabilites::WorkspaceEdit& value) {
    REFLECT_MEMBER_START();
    REFLECT_MEMBER(documentChanges);
    REFLECT_MEMBER_END();
  }

  template<typename TVisitor>
  void Reflect(TVisitor& visitor, WorkspaceClientCapabilites::GenericDynamicReg& value) {
    REFLECT_MEMBER_START();
    REFLECT_MEMBER(dynamicRegistration);
    REFLECT_MEMBER_END();
  }

  template<typename TVisitor>
  void Reflect(TVisitor& visitor, WorkspaceClientCapabilites& value) {
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
  struct TextDocumentClientCapabilities {
    struct Synchronization {
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

    Synchronization synchronization;

    struct Completion {
      // Whether completion supports dynamic registration.
      optional<bool> dynamicRegistration;

      struct CompletionItem {
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
      optional<CompletionItem> completionItem;
    };
    // Capabilities specific to the `textDocument/completion`
    optional<Completion> completion;

    struct GenericDynamicReg {
      // Whether foo supports dynamic registration.
      optional<bool> dynamicRegistration;
    };

    // Capabilities specific to the `textDocument/hover`
    optional<GenericDynamicReg> hover;

    // Capabilities specific to the `textDocument/signatureHelp`
    optional<GenericDynamicReg> signatureHelp;

    // Capabilities specific to the `textDocument/references`
    optional<GenericDynamicReg> references;

    // Capabilities specific to the `textDocument/documentHighlight`
    optional<GenericDynamicReg> documentHighlight;

    // Capabilities specific to the `textDocument/documentSymbol`
    optional<GenericDynamicReg> documentSymbol;

    // Capabilities specific to the `textDocument/formatting`
    optional<GenericDynamicReg> formatting;

    // Capabilities specific to the `textDocument/rangeFormatting`
    optional<GenericDynamicReg> rangeFormatting;

    // Capabilities specific to the `textDocument/onTypeFormatting`
    optional<GenericDynamicReg> onTypeFormatting;

    // Capabilities specific to the `textDocument/definition`
    optional<GenericDynamicReg> definition;

    // Capabilities specific to the `textDocument/codeAction`
    optional<GenericDynamicReg> codeAction;

    // Capabilities specific to the `textDocument/codeLens`
    optional<GenericDynamicReg> codeLens;

    // Capabilities specific to the `textDocument/documentLink`
    optional<GenericDynamicReg> documentLink;

    // Capabilities specific to the `textDocument/rename`
    optional<GenericDynamicReg> rename;
  };

  template<typename TVisitor>
  void Reflect(TVisitor& visitor, TextDocumentClientCapabilities::Synchronization& value) {
    REFLECT_MEMBER_START();
    REFLECT_MEMBER(dynamicRegistration);
    REFLECT_MEMBER(willSave);
    REFLECT_MEMBER(willSaveWaitUntil);
    REFLECT_MEMBER(didSave);
    REFLECT_MEMBER_END();
  }

  template<typename TVisitor>
  void Reflect(TVisitor& visitor, TextDocumentClientCapabilities::Completion& value) {
    REFLECT_MEMBER_START();
    REFLECT_MEMBER(dynamicRegistration);
    REFLECT_MEMBER(completionItem);
    REFLECT_MEMBER_END();
  }

  template<typename TVisitor>
  void Reflect(TVisitor& visitor, TextDocumentClientCapabilities::Completion::CompletionItem& value) {
    REFLECT_MEMBER_START();
    REFLECT_MEMBER(snippetSupport);
    REFLECT_MEMBER_END();
  }

  template<typename TVisitor>
  void Reflect(TVisitor& visitor, TextDocumentClientCapabilities::GenericDynamicReg& value) {
    REFLECT_MEMBER_START();
    REFLECT_MEMBER(dynamicRegistration);
    REFLECT_MEMBER_END();
  }

  template<typename TVisitor>
  void Reflect(TVisitor& visitor, TextDocumentClientCapabilities& value) {
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

  struct ClientCapabilities {
    // Workspace specific client capabilities.
    optional<WorkspaceClientCapabilites> workspace;

    // Text document specific client capabilities.
    optional<TextDocumentClientCapabilities> textDocument;

    /**
    * Experimental client capabilities.
    */
    // experimental?: any; // TODO
  };

  template<typename TVisitor>
  void Deserialize(TVisitor& visitor, ClientCapabilities& value) {
    REFLECT_MEMBER_START();
    REFLECT_MEMBER(workspace);
    REFLECT_MEMBER(textDocument);
    REFLECT_MEMBER_END();
  }

  struct InitializeParams {
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
    optional<DocumentUri> rootUri;

    // User provided initialization options.
    // initializationOptions?: any; // TODO

    // The capabilities provided by the client (editor or tool)
    ClientCapabilities capabilities;

    enum class Trace {
      // NOTE: serialized as a string, one of 'off' | 'messages' | 'verbose';
      Off, // off
      Messages, // messages
      Verbose // verbose
    };

    // The initial trace setting. If omitted trace is disabled ('off').
    Trace trace = Trace::Off;
  };

  void Deserialize(const Reader& reader, InitializeParams::Trace& value) {
    std::string v = reader.GetString();
    if (v == "off")
      value = InitializeParams::Trace::Off;
    else if (v == "messages")
      value = InitializeParams::Trace::Messages;
    else if (v == "verbose")
      value = InitializeParams::Trace::Verbose;
  }

  template<typename TVisitor>
  void Reflect(TVisitor& visitor, InitializeParams& value) {
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
  export namespace InitializeError {
    /**
     * If the protocol version provided by the client can't be handled by the server.
     * @deprecated This initialize error got replaced by client capabilities. There is
     * no version handshake in version 3.0x
     */
    export const unknownProtocolVersion : number = 1;
  }
#endif

  struct InitializeError {
    // Indicates whether the client should retry to send the
    // initilize request after showing the message provided
    // in the ResponseError.
    bool retry;
  };

  template<typename TVisitor>
  void Reflect(TVisitor& visitor, InitializeError& value) {
    REFLECT_MEMBER_START();
    REFLECT_MEMBER(retry);
    REFLECT_MEMBER_END();
  }

  // Defines how the host (editor) should sync document changes to the language server.
  enum class TextDocumentSyncKind {
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

  void Reflect(Writer& writer, TextDocumentSyncKind& value) {
    writer.Int(static_cast<int>(value));
  }

  // Completion options.
  struct CompletionOptions {
    // The server provides support to resolve additional
    // information for a completion item.
    bool resolveProvider = false;

    // The characters that trigger completion automatically.
    std::vector<std::string> triggerCharacters;
  };

  template<typename TVisitor>
  void Reflect(TVisitor& visitor, CompletionOptions& value) {
    REFLECT_MEMBER_START();
    REFLECT_MEMBER(resolveProvider);
    REFLECT_MEMBER(triggerCharacters);
    REFLECT_MEMBER_END();
  }

  // Signature help options.
  struct SignatureHelpOptions {
    // The characters that trigger signature help automatically.
    std::vector<std::string> triggerCharacters;
  };

  template<typename TVisitor>
  void Serialize(TVisitor& visitor, SignatureHelpOptions& value) {
    REFLECT_MEMBER_START();
    writer.StartObject();
    SERIALIZE_MEMBER(triggerCharacters);
    writer.EndObject();
  }

  // Code Lens options.
  struct CodeLensOptions {
    // Code lens has a resolve provider as well.
    bool resolveProvider = false;
  };

  void Serialize(Writer& writer, const CodeLensOptions& value) {
    writer.StartObject();
    SERIALIZE_MEMBER(resolveProvider);
    writer.EndObject();
  }

  // Format document on type options
  struct DocumentOnTypeFormattingOptions {
    // A character on which formatting should be triggered, like `}`.
    std::string firstTriggerCharacter;

    // More trigger characters.
    std::vector<std::string> moreTriggerCharacter;
  };

  void Serialize(Writer& writer, const DocumentOnTypeFormattingOptions& value) {
    writer.StartObject();
    SERIALIZE_MEMBER(firstTriggerCharacter);
    SERIALIZE_MEMBER(moreTriggerCharacter);
    writer.EndObject();
  }

  // Document link options
  struct DocumentLinkOptions {
    // Document links have a resolve provider as well.
    bool resolveProvider = false;
  };

  void Serialize(Writer& writer, const DocumentLinkOptions& value) {
    writer.StartObject();
    SERIALIZE_MEMBER(resolveProvider);
    writer.EndObject();
  }

  // Execute command options.
  struct ExecuteCommandOptions {
    // The commands to be executed on the server
    std::vector<std::string> commands;
  };

  void Serialize(Writer& writer, const ExecuteCommandOptions& value) {
    writer.StartObject();
    SERIALIZE_MEMBER(commands);
    writer.EndObject();
  }

  // Save options.
  struct SaveOptions {
    // The client is supposed to include the content on save.
    bool includeText = false;
  };

  void Serialize(Writer& writer, const SaveOptions& value) {
    writer.StartObject();
    SERIALIZE_MEMBER(includeText);
    writer.EndObject();
  }

  struct TextDocumentSyncOptions {
    // Open and close notifications are sent to the server.
    bool openClose = false;
    // Change notificatins are sent to the server. See TextDocumentSyncKind.None, TextDocumentSyncKind.Full
    // and TextDocumentSyncKindIncremental.
    optional<TextDocumentSyncKind> change;
    // Will save notifications are sent to the server.
    bool willSave = false;
    // Will save wait until requests are sent to the server.
    bool willSaveWaitUntil = false;
    // Save notifications are sent to the server.
    optional<SaveOptions> save;
  };

  void Serialize(Writer& writer, const TextDocumentSyncOptions& value) {
    writer.StartObject();
    SERIALIZE_MEMBER(openClose);
    SERIALIZE_MEMBER(change);
    SERIALIZE_MEMBER(willSave);
    SERIALIZE_MEMBER(willSaveWaitUntil);
    SERIALIZE_MEMBER(save);
    writer.EndObject();
  }

  struct ServerCapabilities {
    // Defines how text documents are synced. Is either a detailed structure defining each notification or
    // for backwards compatibility the TextDocumentSyncKind number.
    optional<TextDocumentSyncOptions> textDocumentSync;
    // The server provides hover support.
    bool hoverProvider = false;
    // The server provides completion support.
    optional<CompletionOptions> completionProvider;
    // The server provides signature help support.
    optional<SignatureHelpOptions> signatureHelpProvider;
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
    optional<CodeLensOptions> codeLensProvider;
    // The server provides document formatting.
    bool documentFormattingProvider = false;
    // The server provides document range formatting.
    bool documentRangeFormattingProvider = false;
    // The server provides document formatting on typing.
    optional<DocumentOnTypeFormattingOptions> documentOnTypeFormattingProvider;
    // The server provides rename support.
    bool renameProvider = false;
    // The server provides document link support.
    optional<DocumentLinkOptions> documentLinkProvider;
    // The server provides execute command support.
    optional<ExecuteCommandOptions> executeCommandProvider;
  };

  void Serialize(Writer& writer, const ServerCapabilities& value) {
    writer.StartObject();
    SERIALIZE_MEMBER(textDocumentSync);
    SERIALIZE_MEMBER(hoverProvider);
    SERIALIZE_MEMBER(completionProvider);
    SERIALIZE_MEMBER(signatureHelpProvider);
    SERIALIZE_MEMBER(definitionProvider);
    SERIALIZE_MEMBER(referencesProvider);
    SERIALIZE_MEMBER(documentHighlightProvider);
    SERIALIZE_MEMBER(documentSymbolProvider);
    SERIALIZE_MEMBER(workspaceSymbolProvider);
    SERIALIZE_MEMBER(codeActionProvider);
    SERIALIZE_MEMBER(codeLensProvider);
    SERIALIZE_MEMBER(documentFormattingProvider);
    SERIALIZE_MEMBER(documentRangeFormattingProvider);
    SERIALIZE_MEMBER(documentOnTypeFormattingProvider);
    SERIALIZE_MEMBER(renameProvider);
    SERIALIZE_MEMBER(documentLinkProvider);
    SERIALIZE_MEMBER(executeCommandProvider);
    writer.EndObject();
  }

  struct InitializeResult {
    // The capabilities the language server provides.
    ServerCapabilities capabilities;
  };

  void Serialize(Writer& writer, const InitializeResult& value) {
    writer.StartObject();
    SERIALIZE_MEMBER(capabilities);
    writer.EndObject();
  }


  struct In_InitializeRequest : public InRequestMessage {
    const static MethodId kMethod = MethodId::Initialize;
    InitializeParams params;

    In_InitializeRequest(optional<RequestId> id, const Reader& reader)
      : InRequestMessage(kMethod, id, reader) {
      auto type = reader.GetType();
      Deserialize(reader, params);
      std::cerr << "done" << std::endl;
    }
  };

  struct Out_InitializeResponse : public OutResponseMessage {
    InitializeResult result;

    // OutResponseMessage:
    void WriteResult(Writer& writer) override {
      Serialize(writer, result);
    }
  };

  struct In_InitializedNotification : public InNotificationMessage {
    const static MethodId kMethod = MethodId::Initialized;

    In_InitializedNotification(optional<RequestId> id, const Reader& reader)
      : InNotificationMessage(kMethod, id, reader) {}
  };














































  struct DocumentSymbolParams {
    TextDocumentIdentifier textDocument;
  };

  void Deserialize(const Reader& reader, DocumentSymbolParams& value) {
    DESERIALIZE_MEMBER(textDocument);
  }

  struct In_DocumentSymbolRequest : public InRequestMessage {
    const static MethodId kMethod = MethodId::TextDocumentDocumentSymbol;

    DocumentSymbolParams params;

    In_DocumentSymbolRequest(optional<RequestId> id, const Reader& reader)
      : InRequestMessage(kMethod, id, reader) {
      Deserialize(reader, params);
    }
  };


  struct Out_DocumentSymbolResponse : public OutResponseMessage {
    std::vector<SymbolInformation> result;

    // OutResponseMessage:
    void WriteResult(Writer& writer) override {
      ::Serialize(writer, result);
    }
  };









  struct WorkspaceSymbolParams {
    std::string query;
  };

  void Deserialize(const Reader& reader, WorkspaceSymbolParams& value) {
    DESERIALIZE_MEMBER(query);
  }

  struct In_WorkspaceSymbolRequest : public InRequestMessage {
    const static MethodId kMethod = MethodId::WorkspaceSymbol;

    WorkspaceSymbolParams params;

    In_WorkspaceSymbolRequest(optional<RequestId> id, const Reader& reader)
      : InRequestMessage(kMethod, id, reader) {
      Deserialize(reader, params);
    }
  };


  struct Out_WorkspaceSymbolResponse : public OutResponseMessage {
    std::vector<SymbolInformation> result;

    // OutResponseMessage:
    void WriteResult(Writer& writer) override {
      ::Serialize(writer, result);
    }
  };

































  enum class MessageType {
    Error = 1,
    Warning = 2,
    Info = 3,
    Log = 4
  };

  void Serialize(Writer& writer, MessageType value) {
    writer.Int(static_cast<int>(value));
  }

  struct ShowMessageOutNotification : public OutNotificationMessage {
    MessageType type = MessageType::Error;
    std::string message;

    // OutNotificationMessage:
    std::string Method() override {
      return "window/showMessage";
    }
    void SerializeParams(Writer& writer) override {
      auto& value = *this;

      writer.StartObject();
      SERIALIZE_MEMBER(type);
      SERIALIZE_MEMBER(message);
      writer.EndObject();
    }
  };

  struct LogMessageOutNotification : public OutNotificationMessage {
    MessageType type = MessageType::Error;
    std::string message;

    // OutNotificationMessage:
    std::string Method() override {
      return "window/logMessage";
    }
    void SerializeParams(Writer& writer) override {
      auto& value = *this;

      writer.StartObject();
      SERIALIZE_MEMBER(type);
      SERIALIZE_MEMBER(message);
      writer.EndObject();
    }
  };


//#undef SERIALIZE_MEMBER
//#undef SERIALIZE_MEMBER2
//#undef DESERIALIZE_MEMBER



}
