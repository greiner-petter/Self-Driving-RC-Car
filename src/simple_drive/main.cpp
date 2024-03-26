#include "../common/ocAlarm.h"
#include "../common/ocHistoryBuffer.h"
#include "../common/ocMember.h"
#include "../common/ocPollEngine.h"
#include "../common/ocTime.h"

#include <cstdint>
#include <iostream>

#define SUBSCRIBE_TO_ANSWER

// https://doc.cup.ostfalia.de/middleware/can/#set_task
struct write_thingy {
  int16_t speed;
  int8_t steer_front; // 0 = gerade
  int8_t steer_rear;
  int8_t id; // individual task id
  int32_t distance; // distance in ticks
} __attribute__((packed));

int main() {
  ocMember member(ocMemberId::Simple_Drive, "Simple_Drive");
  member.attach();
  ocIpcSocket *socket = member.get_socket();

#ifdef SUBSCRIBE_TO_ANSWER
  ocPacket subscribe(ocMessageId::Subscribe_To_Messages);
  subscribe.clear_and_edit().write(ocMessageId::Driving_Task_Finished);
  socket->send_packet(subscribe);
#endif

  ocPacket received;

  struct write_thingy a = {
    .speed = 100,
    .steer_front = 0,
    .steer_rear = 0,
    .id = -1,
    .distance = 0,
  };

  while (1) {
    getchar();

    a.distance += 4096;

    ocPacket s(ocMessageId::Start_Driving_Task);
    s.edit_from_end().write(&a, sizeof(struct write_thingy));
    std::cout << "Result is: " << socket->send_packet(s) << std::endl;

#ifdef SUBSCRIBE_TO_ANSWER
    // if speed has the value 8 there is no answer?!?
    // with the current value the IPC only answers 10 times...
    std::cout << "Got answer: " << socket->read_packet(received);
#endif
  }
  return 0;
}
