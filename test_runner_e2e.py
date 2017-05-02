import shlex
from subprocess import Popen, PIPE

# We write test files in python. The test runner collects all python files in
# the directory and executes them. The test function just creates a test object
# which specifies expected stdin/stdout.
#
# Test functions are automatically discovered; they just need to be in the
# global environment and start with `Test_`.

class TestBuilder:
  def WithFile(self, filename, contents):
    """
    Writes the file contents to disk so that the language server can access it.
    """
    pass

  def Send(self, stdin):
    """
    Send the given message to the language server.
    """

    # Content-Length: ...\r\n
    # \r\n
    # {
    #   "jsonrpc": "2.0",
    #   "id": 1,
    #   "method": "textDocument/didOpen",
    #   "params": {
    #     ...
    #   }
    # }

  def Expect(self, stdout):
    """
    Expect a message from the language server.
    """
    pass

  def SetupCommonInit():
    """
    Add initialize/initialized messages.
    """
    pass


def Test_Outline():
  return TestBuilder()
    .SetupCommonInit()
    .WithFile("foo.cc",
      """
      void main() {}
      """
    )
    .Send({
      'id': 1,
      'method': 'textDocument/documentSymbol',
      'params': {}
    })
    .Expect({
      'id': 1
      'result': [
        lsSymbolInfo('void main()', (1, 1), lsSymbolKind.Function)
      ]
    })


# Possible test runner implementation
# cmd = "x64/Release/indexer.exe --language-server"
# process = Popen(shlex.split(cmd), stdin=PIPE, stdout=PIPE)
# process.communicate('{}')
# exit_code = process.wait()