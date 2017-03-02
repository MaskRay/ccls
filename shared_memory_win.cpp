#include <iostream>
#include <vector>
#include <memory>

#include "ipc.h"



int main525252(int argc, char** argv) {
  if (argc == 2) {
    IpcMessageQueue queue("myqueue");
    int i = 0;
    while (true) {
      IpcMessage_ImportIndex m;
      m.path = "foo #" + std::to_string(i);
      queue.PushMessage(&m);
      std::cout << "Sent " << i << std::endl;;

      using namespace std::chrono_literals;
      std::this_thread::sleep_for(10ms);

      ++i;
    }
  }

  else {
    IpcMessageQueue queue("myqueue");

    while (true) {
      std::vector<std::unique_ptr<BaseIpcMessage>> messages = queue.PopMessage();
      std::cout << "Got " << messages.size() << " messages" << std::endl;

      for (auto& message : messages) {
        rapidjson::StringBuffer output;
        rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(output);
        writer.SetFormatOptions(
          rapidjson::PrettyFormatOptions::kFormatSingleLineArray);
        writer.SetIndent(' ', 2);
        message->Serialize(writer);

        std::cout << "  kind=" << static_cast<int>(message->kind) << ", json=" << output.GetString() << std::endl;
      }

      using namespace std::chrono_literals;
      std::this_thread::sleep_for(5s);
    }
  }

  return 0;
}