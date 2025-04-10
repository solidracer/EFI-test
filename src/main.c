#include <efi.h>
#include <efilib.h>

#define KENTRY 0x100000

typedef void (*KENTRY_T)(void);

#define KERNELNAME L"\\kernel.bin"
#define ERRCHECK(errmsg, check) \
if (EFI_ERROR(check)) { \
    Print(L"%s: %r\r\n", errmsg, check); \
    return check; \
}

static EFI_STATUS pauseConsole(VOID) {
    EFI_STATUS stat;

    Print(L"Waiting for keyboard input before proceeding...\r\n");

    stat = gBS->WaitForEvent(1, &gST->ConIn->WaitForKey, NULL);
    if (EFI_ERROR(stat)) return stat;

    stat = gST->ConIn->ReadKeyStroke(gST->ConIn, NULL);
    if (EFI_ERROR(stat)) return stat;

    return EFI_SUCCESS;
}

static EFI_STATUS getFSProtocol(EFI_SIMPLE_FILE_SYSTEM_PROTOCOL **fs) {
    EFI_GUID sfspGuid = EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_GUID;
    EFI_HANDLE *handles = NULL;
    UINTN handleCount = 0;
    EFI_STATUS efiStatus;
    efiStatus = gBS->LocateHandleBuffer(ByProtocol, &sfspGuid, NULL, &handleCount, &handles);
    if (EFI_ERROR(efiStatus)) return efiStatus;
    for (INT32 index = 0; index < (INT32)handleCount; index++) {
        efiStatus = gBS->HandleProtocol(
            handles[index],
            &sfspGuid,
            (VOID**)fs);
        if (!EFI_ERROR(efiStatus)) break;
    }
    return efiStatus;
}

EFI_STATUS efi_main(EFI_HANDLE imageHandle, EFI_SYSTEM_TABLE *systemTable) {
    InitializeLib(imageHandle, systemTable);

    Print(L"Welcome to ");
    gST->ConOut->SetAttribute(gST->ConOut, EFI_CYAN);
    Print(L"ZenithOS");
    gST->ConOut->SetAttribute(gST->ConOut, EFI_LIGHTGRAY);
    Print(L" Bootloader!\r\n\r\n");

    pauseConsole();
    /* i can add quiet in the future maybe */
    Print(L"Output: VERBOSE\r\n");

    EFI_STATUS stat;
    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *fs = NULL; 
    stat = getFSProtocol(&fs);
    ERRCHECK( "getFSProtocol():", stat );
    
    Print(L"Found EFI FS Protocol for ESP...\r\n");

    EFI_FILE_PROTOCOL *root = NULL;
    stat = fs->OpenVolume(fs, &root);
    ERRCHECK( L"fs->OpenVolume()", stat );

    Print(L"ESP Volume opened...\r\n");

    EFI_FILE_PROTOCOL *kernel_file = NULL;

    stat = root->Open( root, &kernel_file, KERNELNAME, EFI_FILE_MODE_READ, EFI_FILE_READ_ONLY);
    ERRCHECK( L"root->Open()", stat );

    Print(L"Found the kernel file...\r\n");

    EFI_FILE_INFO *info = NULL;
    UINTN size = 0;

    for (UINT8 i = 0;i<2;i++) {
        if (size) {
            Print(L"Got the size from last call (%d B), allocating info structure...\r\n", size);
            info = (EFI_FILE_INFO*) AllocatePool(size);
        }
        stat = kernel_file->GetInfo(kernel_file, &gEfiFileInfoGuid, &size, (VOID*)info);
        Print(L"kernel->GetInfo(): %r\r\n", stat);
    }
    ERRCHECK( L"kernel->GetInfo()", stat );

    Print(L"Kernel Name: %s\r\nFile Size: %d\r\n", info->FileName, info->FileSize);

    UINTN kernel_addr = KENTRY;
    stat = gBS->AllocatePages(AllocateAddress, EfiLoaderData, EFI_SIZE_TO_PAGES(info->FileSize), &kernel_addr);
    ERRCHECK( "gBS->AllocatePages()" , stat )

    stat = kernel_file->Read(kernel_file, &info->FileSize, (VOID*)KENTRY);
    ERRCHECK( L"kernel->Read()", stat );

    Print(L"Read byte count: %d\r\nKernel loaded at: %p\r\n", info->FileSize, KENTRY);

    Print(L"Cleaning up allocated data...\r\n");
    FreePool(info);

    Print(L"Closing files...\r\n");
    kernel_file->Close(kernel_file);
    root->Close(root);

    UINTN mapSize, mapKey, descSize;
    UINT32 descVersion;
    EFI_MEMORY_DESCRIPTOR *memoryMap = NULL;

    stat = gBS->GetMemoryMap(&mapSize, memoryMap, &mapKey, &descSize, &descVersion);

    stat = gBS->AllocatePool(EfiLoaderData, mapSize, (VOID**)&memoryMap);

    stat = gBS->GetMemoryMap(&mapSize, memoryMap, &mapKey, &descSize, &descVersion);

    stat = gBS->ExitBootServices(imageHandle, mapKey);
    ERRCHECK( "gBS->ExitBootServices() ", stat );

    KENTRY_T entry = (KENTRY_T)KENTRY;
    entry();

    __builtin_unreachable();

    return EFI_SUCCESS;
}
