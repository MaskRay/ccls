import json
import shlex
from subprocess import Popen, PIPE

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

# We write test files in python. The test runner collects all python files in
# the directory and executes them. The test function just creates a test object
# which specifies expected stdin/stdout.
#
# Test functions are automatically discovered; they just need to be in the
# global environment and start with `Test_`.

class TestBuilder:
  def __init__(self):
    self.files = []
    self.sent = []
    self.received = []

  def WithFile(self, filename, contents):
    """
    Writes the file contents to disk so that the language server can access it.
    """
    self.files.append((filename, contents))
    return self

  def Send(self, stdin):
    """
    Send the given message to the language server.
    """
    stdin['jsonrpc'] = '2.0'
    self.sent.append(stdin)
    return self

  def Expect(self, stdout):
    """
    Expect a message from the language server.
    """
    self.received.append(stdout)
    return self

  def SetupCommonInit(self):
    """
    Add initialize/initialized messages.
    """
    self.Send({
      'id': 0,
      'method': 'initialize',
      'params': {
        'processId': 123,
        'rootPath': 'cquery',
        'capabilities': {},
        'trace': 'off'
      }
    })
    self.Expect({
      'id': 0,
      'method': 'initialized',
      'result': {}
    })
    return self

def _ExecuteTest(name, func):
  """
  Executes a specific test.

  |func| must return a TestBuilder object.
  """
  test_builder = func()
  if not isinstance(test_builder, TestBuilder):
    raise Exception('%s does not return a TestBuilder instance' % name)

  test_builder.Send({ 'method': 'exit' })

  # Possible test runner implementation
  cmd = "x64/Debug/indexer.exe --language-server"
  process = Popen(shlex.split(cmd), stdin=PIPE, stdout=PIPE, stderr=PIPE, universal_newlines=True)

  stdin = ''
  for message in test_builder.sent:
    payload = json.dumps(message)
    wrapped = 'Content-Length: %s\r\n\r\n%s' % (len(payload), payload)
    stdin += wrapped

  print('## %s ##' % name)
  print('== STDIN ==')
  print(stdin)
  (stdout, stderr) = process.communicate(stdin)
  if stdout:
    print('== STDOUT ==')
    print(stdout)
  if stderr:
    print('== STDERR ==')
    print(stderr)

  # TODO: Actually verify stdout.

  exit_code = process.wait()

def _DiscoverTests():
  """
  Discover and return all tests.
  """
  for name, value in globals().items():
    if not callable(value):
      continue
    if not name.startswith('Test_'):
      continue
    yield (name, value)

def _RunTests():
  """
  Executes all tests.
  """
  for name, func in _DiscoverTests():
    print('Running test function %s' % name)
    _ExecuteTest(name, func)




class lsSymbolKind:
  Function = 1

def lsSymbolInfo(name, position, kind):
  return {
    'name': name,
    'position': position,
    'kind': kind
  }

def Test_Init():
  return (TestBuilder()
    .SetupCommonInit()
  )

def _Test_Outline():
  return (TestBuilder()
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
      'id': 1,
      'result': [
        lsSymbolInfo('void main()', (1, 1), lsSymbolKind.Function)
      ]
    }))


if __name__ == '__main__':
  _RunTests()