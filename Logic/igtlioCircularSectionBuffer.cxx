/*==========================================================================

  Portions (c) Copyright 2008-2009 Brigham and Women's Hospital (BWH) All Rights Reserved.

  See Doc/copyright/copyright.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

==========================================================================*/

// VTK includes
#include "igtlioCircularSectionBuffer.h"

#include <vtkObjectFactory.h>
#include <vtksys/SystemTools.hxx>

// OpenIGTLink includes
#include <igtlMessageBase.h>

// OpenIGTLinkIO includes
#include "igtlioUtilities.h"

// STD includes
#include <algorithm>
#include <string>

//---------------------------------------------------------------------------
vtkStandardNewMacro(igtlioCircularSectionBuffer);

//---------------------------------------------------------------------------
igtlioCircularSectionBuffer::igtlioCircularSectionBuffer() :
  PacketMode(SinglePacketMode),
  DroppedMessages(0)
{
  this->BufferSize = IGTLCB_SECTION_BUFFER_SIZE;
  this->Initialization();
}

//---------------------------------------------------------------------------
int igtlioCircularSectionBuffer::Initialization()
{
  std::lock_guard<std::recursive_mutex> lock(this->Mutex);
  this->PushMessage = NULL;
  this->Received.clear();
  this->InPull.clear();
  this->DroppedMessages = 0;
  return 1;
}

//---------------------------------------------------------------------------
igtlioCircularSectionBuffer::~igtlioCircularSectionBuffer()
{
}

//---------------------------------------------------------------------------
void igtlioCircularSectionBuffer::PrintSelf(ostream& os, vtkIndent indent)
{
  this->vtkObject::PrintSelf(os, indent);
}

//---------------------------------------------------------------------------
// Functions to push data into the buffer (for receiving thread)
//
//   StartPush() :     Prepare a new message buffer
//   GetPushBuffer():  Get the message buffer to receive into
//   EndPush() :       Finish pushing data. The message becomes ready to
//                     be read by the main thread.
//---------------------------------------------------------------------------
int igtlioCircularSectionBuffer::StartPush()
{
  std::lock_guard<std::recursive_mutex> lock(this->Mutex);
  this->PushMessage = igtl::MessageBase::New();
  this->PushMessage->InitPack();
  return 0;
}

//---------------------------------------------------------------------------
igtl::MessageBase::Pointer igtlioCircularSectionBuffer::GetPushBuffer()
{
  std::lock_guard<std::recursive_mutex> lock(this->Mutex);
  return this->PushMessage;
}

//---------------------------------------------------------------------------
void igtlioCircularSectionBuffer::EndPush()
{
  std::lock_guard<std::recursive_mutex> lock(this->Mutex);
  if (this->PushMessage)
    {
    this->Received.push_back(this->PushMessage);
    this->PushMessage = NULL;
    }
  size_t maxSize = (this->PacketMode == SinglePacketMode) ? 1 : static_cast<size_t>(std::max(this->BufferSize, 1));
  while (this->Received.size() > maxSize)
    {
    this->Received.pop_front();
    ++this->DroppedMessages;
    }
}

//---------------------------------------------------------------------------
// Functions to pull data from the buffer (for main thread)
//
//   StartPull() :     Take all received messages
//   GetPullBuffer():  Get the next taken message, oldest first
//   EndPull() :       Finish pulling data
//---------------------------------------------------------------------------
int igtlioCircularSectionBuffer::StartPull()
{
  std::lock_guard<std::recursive_mutex> lock(this->Mutex);
  this->InPull.insert(this->InPull.end(), this->Received.begin(), this->Received.end());
  this->Received.clear();
  return static_cast<int>(this->InPull.size());
}

//---------------------------------------------------------------------------
igtl::MessageBase::Pointer igtlioCircularSectionBuffer::GetPullBuffer()
{
  igtl::MessageBase::Pointer retval;
  if (!this->InPull.empty())
    {
    retval = this->InPull.front();
    this->InPull.pop_front();
    }
  return retval;
}

//---------------------------------------------------------------------------
void igtlioCircularSectionBuffer::EndPull()
{
  this->InPull.clear();
}

//---------------------------------------------------------------------------
int igtlioCircularSectionBuffer::IsUpdated()
{
  std::lock_guard<std::recursive_mutex> lock(this->Mutex);
  return this->Received.empty() ? 0 : 1;
}

//---------------------------------------------------------------------------
bool igtlioCircularSectionBuffer::IsSectionBufferInProcess()
{
  return !this->InPull.empty();
}

//---------------------------------------------------------------------------
int igtlioCircularSectionBuffer::GetNumberOfDroppedMessages()
{
  std::lock_guard<std::recursive_mutex> lock(this->Mutex);
  return this->DroppedMessages;
}
