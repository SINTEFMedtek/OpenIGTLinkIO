#include <iostream>
#include <string>
#include <thread>
#include <atomic>
#include <vector>
#include <vtkSmartPointer.h>
#include "igtlioCircularSectionBuffer.h"

namespace
{

int failures = 0;

void check(bool condition, const std::string& description)
{
  if (!condition)
    {
    std::cout << "FAILURE: " << description << std::endl;
    ++failures;
    }
}

void push(igtlioCircularSectionBuffer* buffer, int id)
{
  buffer->StartPush();
  igtl::MessageBase::Pointer message = buffer->GetPushBuffer();
  message->SetDeviceName(std::to_string(id));
  buffer->EndPush();
}

std::vector<int> pullAll(igtlioCircularSectionBuffer* buffer)
{
  std::vector<int> retval;
  buffer->StartPull();
  while (buffer->IsSectionBufferInProcess())
    {
    retval.push_back(std::stoi(buffer->GetPullBuffer()->GetDeviceName()));
    }
  buffer->EndPull();
  return retval;
}

void testMultiplePacketsKeepsTheNewestInOrder()
{
  vtkSmartPointer<igtlioCircularSectionBuffer> buffer = vtkSmartPointer<igtlioCircularSectionBuffer>::New();
  buffer->SetPacketMode(igtlioCircularSectionBuffer::MultiplePacketsMode);
  buffer->SetBufferSize(10);
  check(!buffer->IsUpdated(), "a new buffer has no messages");
  for (int i = 0; i < 15; ++i)
    {
    push(buffer, i);
    }
  check(buffer->IsUpdated(), "pushed messages are ready to pull");
  std::vector<int> pulled = pullAll(buffer);
  check(pulled.size() == 10, "the buffer keeps BufferSize messages");
  for (unsigned i = 0; i < pulled.size(); ++i)
    {
    check(pulled[i] == int(i) + 5, "the newest messages are pulled, oldest first");
    }
  check(buffer->GetNumberOfDroppedMessages() == 5, "the oldest messages are counted as dropped");
  check(!buffer->IsUpdated(), "pulled messages are not pulled again");
}

void testSinglePacketKeepsOnlyTheNewest()
{
  vtkSmartPointer<igtlioCircularSectionBuffer> buffer = vtkSmartPointer<igtlioCircularSectionBuffer>::New();
  buffer->SetPacketMode(igtlioCircularSectionBuffer::SinglePacketMode);
  push(buffer, 1);
  push(buffer, 2);
  push(buffer, 3);
  std::vector<int> pulled = pullAll(buffer);
  check(pulled.size() == 1 && pulled[0] == 3, "single packet mode keeps only the newest message");
  check(buffer->GetNumberOfDroppedMessages() == 2, "single packet mode counts the replaced messages");
}

void testMessagesPushedDuringPullAreKept()
{
  vtkSmartPointer<igtlioCircularSectionBuffer> buffer = vtkSmartPointer<igtlioCircularSectionBuffer>::New();
  buffer->SetPacketMode(igtlioCircularSectionBuffer::MultiplePacketsMode);
  push(buffer, 1);
  push(buffer, 2);
  buffer->StartPull();
  igtl::MessageBase::Pointer first = buffer->GetPullBuffer();
  push(buffer, 3);
  check(first->GetDeviceName() == std::string("1"), "a pulled message is not changed by a later push");
  igtl::MessageBase::Pointer second = buffer->GetPullBuffer();
  check(second->GetDeviceName() == std::string("2"), "the pulled section is pulled in order");
  check(!buffer->IsSectionBufferInProcess(), "the pulled section ends with the messages it started with");
  buffer->EndPull();
  std::vector<int> pulled = pullAll(buffer);
  check(pulled.size() == 1 && pulled[0] == 3, "a message pushed during a pull is pulled next time");
}

void testConcurrentPushAndPull()
{
  vtkSmartPointer<igtlioCircularSectionBuffer> buffer = vtkSmartPointer<igtlioCircularSectionBuffer>::New();
  buffer->SetPacketMode(igtlioCircularSectionBuffer::MultiplePacketsMode);
  buffer->SetBufferSize(10);
  const int count = 100000;
  std::atomic<bool> done(false);
  std::thread receiver([&]()
    {
    for (int i = 0; i < count; ++i)
      {
      push(buffer, i);
      }
    done = true;
    });
  int pulledCount = 0;
  int last = -1;
  bool ordered = true;
  bool finished = false;
  while (!finished)
    {
    finished = done;
    std::vector<int> pulled = pullAll(buffer);
    for (unsigned i = 0; i < pulled.size(); ++i)
      {
      ordered = ordered && pulled[i] > last;
      last = pulled[i];
      }
    pulledCount += int(pulled.size());
    }
  receiver.join();
  pulledCount += int(pullAll(buffer).size());
  check(ordered, "concurrently pushed messages are pulled in order");
  check(last == count - 1, "the last pushed message is pulled");
  check(pulledCount + buffer->GetNumberOfDroppedMessages() == count, "every message is either pulled or counted as dropped");
}

}

int main(int, char**)
{
  testMultiplePacketsKeepsTheNewestInOrder();
  testSinglePacketKeepsOnlyTheNewest();
  testMessagesPushedDuringPullAreKept();
  testConcurrentPushAndPull();
  if (failures == 0)
    {
    std::cout << "testCircularSectionBuffer: all tests passed" << std::endl;
    }
  return failures == 0 ? 0 : 1;
}
