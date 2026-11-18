/*==========================================================================

  Portions (c) Copyright 2008-2009 Brigham and Women's Hospital (BWH) All Rights Reserved.

  See Doc/copyright/copyright.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

==========================================================================*/

#ifndef IGTLIOCIRCULARSECTIONBUFFER_H
#define IGTLIOCIRCULARSECTIONBUFFER_H

// STL includes
#include <deque>
#include <string>
#include <mutex>

// VTK includes
#include <vtkObject.h>

// OpenIGTLink includes
#include <igtlMessageBase.h>

// Local includes
#include "igtlioLogicExport.h"

// Maximum number of received messages kept per device until they are read: 110 ms at 45 messages/s
#define IGTLCB_SECTION_BUFFER_SIZE             5
#define IGTLCB_CIRC_BUFFER_SIZE_VIDEOSTREAM   50

#ifndef VTK_OVERRIDE
#define VTK_OVERRIDE override
#endif

/// Received messages of one device, from the receiving thread to the main thread.
///
/// Each message has its own buffer, so the receiving thread never writes into a
/// message that the main thread is reading. In MultiplePacketsMode all messages are
/// kept, up to BufferSize, after which the oldest are dropped. In SinglePacketMode
/// only the newest message is kept.
class OPENIGTLINKIO_LOGIC_EXPORT igtlioCircularSectionBuffer : public vtkObject
{
 public:

  static igtlioCircularSectionBuffer *New();
  vtkTypeMacro(igtlioCircularSectionBuffer, vtkObject);

  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  int GetNumberOfBuffer() { return this->BufferSize; }

  // Receiving thread
  int            StartPush();
  void           EndPush();
  igtl::MessageBase::Pointer GetPushBuffer();

  // Main thread
  int            StartPull();
  void           EndPull();
  igtl::MessageBase::Pointer GetPullBuffer();

  int            IsUpdated();

  int            Initialization();

  enum
  {
  SinglePacketMode = 0,
  MultiplePacketsMode = 1
  };

  vtkSetMacro( PacketMode, int);
  vtkGetMacro( PacketMode, int);

  vtkSetMacro( BufferSize, int);
  vtkGetMacro( BufferSize, int);

  bool IsSectionBufferInProcess();

  /// Messages dropped because they were not read before newer messages replaced them
  int GetNumberOfDroppedMessages();

 protected:
   igtlioCircularSectionBuffer();
   virtual ~igtlioCircularSectionBuffer();

 protected:

  std::recursive_mutex         Mutex;
  int                PacketMode;
  int                BufferSize;
  int                DroppedMessages;

  igtl::MessageBase::Pointer PushMessage;            // being received, receiving thread only
  std::deque<igtl::MessageBase::Pointer> Received;   // received and not yet pulled
  std::deque<igtl::MessageBase::Pointer> InPull;     // pulled section, main thread only
};

#endif //IGTLIOCIRCULARSECTIONBUFFER_H
