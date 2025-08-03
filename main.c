#include "nyx.h"
#include <Uefi.h>
#include <Library/UefiLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Protocol/LoadedImage.h>
#include <Library/PrintLib.h>

EFI_PHYSICAL_ADDRESS PayloadBufferAddress;
EFI_PHYSICAL_ADDRESS TraceBufferAddress;
EFI_PHYSICAL_ADDRESS IjonTraceBufferAddress;

__attribute__((noinline)) void First(IN EFI_HANDLE ImageHandle) {
  EFI_STATUS state;

  kAFL_hypercall(HYPERCALL_KAFL_LOCK, 0);
  kAFL_hypercall(HYPERCALL_KAFL_ACQUIRE, 0);
  kAFL_hypercall(HYPERCALL_KAFL_RELEASE, 0);
  host_config_t host_config = {0};
  kAFL_hypercall(HYPERCALL_KAFL_GET_HOST_CONFIG, (uintptr_t)&host_config);

  agent_config_t agent_config = {
    .agent_magic = NYX_AGENT_MAGIC,
    .agent_version = NYX_AGENT_VERSION,
    .agent_ijon_tracing = 1,
    .coverage_bitmap_size = host_config.bitmap_size,
    .trace_buffer_vaddr = (uint64_t)TraceBufferAddress,
    .ijon_trace_buffer_vaddr = (uint64_t)IjonTraceBufferAddress,
  };
  kAFL_hypercall(HYPERCALL_KAFL_SET_AGENT_CONFIG, (uintptr_t)&agent_config);
  
  kAFL_hypercall(HYPERCALL_KAFL_GET_PAYLOAD, (uintptr_t)PayloadBufferAddress);
  
  EFI_STATUS Status;
  EFI_LOADED_IMAGE_PROTOCOL *LoadedImage;
  UINT64 AppImageStart = 0;
  UINT64 AppImageEnd = 0;

  Status = gBS->HandleProtocol(
                  ImageHandle,
                  &gEfiLoadedImageProtocolGuid,
                  (VOID **)&LoadedImage
                  );

  if (EFI_ERROR(Status)) {
    Print(L"Error: Could not get Loaded Image Protocol. Status = %r\n", Status);
  } else {
    AppImageStart = (UINT64)LoadedImage->ImageBase;
    AppImageEnd = AppImageStart + LoadedImage->ImageSize;
    Print(L"Application Image Range: 0x%lx - 0x%lx\n", AppImageStart, AppImageEnd);
  }

  uint64_t buffer[3] = {0};
  buffer[0] = AppImageStart; // low range
  buffer[1] = AppImageEnd;   // high range
  buffer[2] = 0;             // IP filter index [0-3]
  
  kAFL_hypercall(HYPERCALL_KAFL_RANGE_SUBMIT, (uint64_t)buffer);
  
  kAFL_hypercall(HYPERCALL_KAFL_SUBMIT_CR3, 0);
  kAFL_hypercall(HYPERCALL_KAFL_NEXT_PAYLOAD, 0);
  kAFL_hypercall(HYPERCALL_KAFL_ACQUIRE, 0);
}

__attribute__((noinline)) void Second() {
  kAFL_hypercall(HYPERCALL_KAFL_RELEASE, 0);
}

EFI_STATUS
EFIAPI
myEfiMain (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  First(ImageHandle); // Take initial snapshot, and exchange metadata

  kAFL_payload *payload_buffer = (kAFL_payload *)PayloadBufferAddress;

  if (payload_buffer->data[0] == 'T')
  {
      if (payload_buffer->data[1] == 'E')
      {
        if (payload_buffer->data[2] == 'S')
        {
          if (payload_buffer->data[3] == 'T')
          {
            kAFL_hypercall(HYPERCALL_KAFL_PANIC_EXTENDED, (uintptr_t) "Something went wrong\n");
          }
        }
      }
  }

  Second();  // Time to restore snapshot

  puts("I'm here, look here."); 

  return 0;
}