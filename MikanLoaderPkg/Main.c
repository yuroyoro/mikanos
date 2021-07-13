#include <Uefi.h>
#include <Library/UefiLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/PrintLib.h>
#include <Protocol/LoadedImage.h>
#include <Protocol/SimpleFileSystem.h>
#include <Protocol/DiskIo2.h>
#include <Protocol/BlockIo.h>
#include <Guid/FileInfo.h>

struct MemoryMap
{
	UINTN buffer_size;
	VOID *buffer;
	UINTN map_size;
	UINTN map_key;
	UINTN descriptor_size;
	UINT32 descriptor_version;
};

EFI_STATUS GetMemoryMap(struct MemoryMap *map)
{
	if (map->buffer == NULL)
	{
		return EFI_BUFFER_TOO_SMALL;
	}

	map->map_size = map->buffer_size;
	return gBS->GetMemoryMap(
		&map->map_size,
		(EFI_MEMORY_DESCRIPTOR *)map->buffer,
		&map->map_key,
		&map->descriptor_size,
		&map->descriptor_version);
}

// table for memory type label
const CHAR16 *GetMemoryTypeUnicode(EFI_MEMORY_TYPE type)
{
	switch (type)
	{
	case EfiReservedMemoryType:
		return L"EfiReservedMemoryType";
	case EfiLoaderCode:
		return L"EfiLoaderCode";
	case EfiLoaderData:
		return L"EfiLoaderData";
	case EfiBootServicesCode:
		return L"EfiBootServicesCode";
	case EfiBootServicesData:
		return L"EfiBootServicesData";
	case EfiRuntimeServicesCode:
		return L"EfiRuntimeServicesCode";
	case EfiRuntimeServicesData:
		return L"EfiRuntimeServicesData";
	case EfiConventionalMemory:
		return L"EfiConventionalMemory";
	case EfiUnusableMemory:
		return L"EfiUnusableMemory";
	case EfiACPIReclaimMemory:
		return L"EfiACPIReclaimMemory";
	case EfiACPIMemoryNVS:
		return L"EfiACPIMemoryNVS";
	case EfiMemoryMappedIO:
		return L"EfiMemoryMappedIO";
	case EfiMemoryMappedIOPortSpace:
		return L"EfiMemoryMappedIOPortSpace";
	case EfiPalCode:
		return L"EfiPalCode";
	case EfiPersistentMemory:
		return L"EfiPersistentMemory";
	case EfiMaxMemoryType:
		return L"EfiMaxMemoryType";
	default:
		return L"InvalidMemoryType";
	}
}

EFI_STATUS SaveMemoryMap(struct MemoryMap *map, EFI_FILE_PROTOCOL *file)
{
	CHAR8 buf[256];
	UINTN len;

	// write csv file header
	CHAR8 *header =
		"Index, Type, Type(name), PhysicalStart, NumberOfPages, Attribute\n";
	len = AsciiStrLen(header);
	file->Write(file, &len, header);

	Print(L"map->buffer = %08lx, map->map_size = %08lx\n", map->buffer, map->map_size);

	EFI_PHYSICAL_ADDRESS iter;
	int i;
	for (
		iter = (EFI_PHYSICAL_ADDRESS)map->buffer, i = 0;
		iter < (EFI_PHYSICAL_ADDRESS)map->buffer + map->map_size;
		iter += map->descriptor_size, i++)
	{
		EFI_MEMORY_DESCRIPTOR *desc = (EFI_MEMORY_DESCRIPTOR *)iter;
		len = AsciiSPrint(
			buf, sizeof(buf),
			"%u, %x, %-ls, %08lx, %lx, %lx\n",
			i, desc->Type, GetMemoryTypeUnicode(desc->Type),
			desc->PhysicalStart, desc->NumberOfPages,
			desc->Attribute & 0xffffflu);
		file->Write(file, &len, buf);
	}

	return EFI_SUCCESS;
}

EFI_STATUS OpenRootDir(EFI_HANDLE image_handle, EFI_FILE_PROTOCOL **root)
{
	EFI_LOADED_IMAGE_PROTOCOL *loaded_image;
	EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *fs;

	gBS->OpenProtocol(
		image_handle,
		&gEfiLoadedImageProtocolGuid,
		(VOID **)&loaded_image,
		image_handle,
		NULL,
		EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL);

	gBS->OpenProtocol(
		loaded_image->DeviceHandle,
		&gEfiSimpleFileSystemProtocolGuid,
		(VOID **)&fs,
		image_handle,
		NULL,
		EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL);

	fs->OpenVolume(fs, root);
	return EFI_SUCCESS;
}

EFI_STATUS LoadKernel(EFI_FILE_PROTOCOL *root_dir, EFI_PHYSICAL_ADDRESS kernel_base_addr)
{
	// open "kernel.elf"
	EFI_FILE_PROTOCOL *kernel_file;
	root_dir->Open(
		root_dir, &kernel_file, L"\\kernel.elf",
		EFI_FILE_MODE_READ, 0);

	// get fileinfo
	UINTN file_info_size = sizeof(EFI_FILE_INFO) + sizeof(CHAR16) * 12;
	UINT8 file_info_buffer[file_info_size];
	kernel_file->GetInfo(
		kernel_file, &gEfiFileInfoGuid,
		&file_info_size, file_info_buffer);

	// file size
	EFI_FILE_INFO *file_info = (EFI_FILE_INFO *)file_info_buffer;
	UINTN kernel_file_size = file_info->FileSize;

	// read kernel into base_addr
	gBS->AllocatePages(
		AllocateAddress, EfiLoaderData,
		(kernel_file_size + 0xfff) / 0x1000, &kernel_base_addr);
	kernel_file->Read(kernel_file, &kernel_file_size, (VOID *)kernel_base_addr);

	Print(L"Loaded Kernel : kernel_base_addr = %08lx, kernel_file_size = %lu\n", kernel_base_addr, kernel_file_size);

	return EFI_SUCCESS;
}

EFI_STATUS ExitBootService(EFI_HANDLE image_handle, struct MemoryMap *map)
{
	EFI_STATUS status;
	status = gBS->ExitBootServices(image_handle, map->map_key);
	if (EFI_ERROR(status))
	{
		status = GetMemoryMap(map);
		if (EFI_ERROR(status))
		{
			Print(L"Failed to get memory map: %r\n", status);
			return status;
		}

		status = gBS->ExitBootServices(image_handle, map->map_key);
		if (EFI_ERROR(status))
		{
			Print(L"Could not exit boot service: %r\n", status);
			return status;
		}
	}

	return EFI_SUCCESS;
}

EFI_STATUS EFIAPI UefiMain(
	EFI_HANDLE image_handle,
	EFI_SYSTEM_TABLE *system_table)
{
	Print(L"Hello, Mikan World!\n");

	// get memroy map
	CHAR8 memmap_buf[4096 * 4];
	struct MemoryMap memmap = {sizeof(memmap_buf), memmap_buf, 0, 0, 0, 0};

	EFI_STATUS ret;
	ret = GetMemoryMap(&memmap);
	if (EFI_ERROR(ret))
	{
		Print(L"Failed to GetMemoryMap: %r\n", ret);
		goto out;
	}

	// save memory map to the file

	EFI_FILE_PROTOCOL *root_dir;
	ret = OpenRootDir(image_handle, &root_dir);
	if (EFI_ERROR(ret))
	{
		Print(L"Failed to OpenRootDir: %r\n", ret);
		goto out;
	}

	EFI_FILE_PROTOCOL *memmap_file;
	root_dir->Open(
		root_dir, &memmap_file, L"\\memmap",
		EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE | EFI_FILE_MODE_CREATE, 0);

	ret = SaveMemoryMap(&memmap, memmap_file);
	if (EFI_ERROR(ret))
	{
		Print(L"Failed to SaveMemoryMap: %r\n", ret);
		goto out;
	}
	memmap_file->Close(memmap_file);

	// load kernel
	EFI_PHYSICAL_ADDRESS kernel_base_addr = 0x100000;
	ret = LoadKernel(root_dir, kernel_base_addr);
	if (EFI_ERROR(ret))
	{
		Print(L"Failed to LoadKernel: %r\n", ret);
		goto out;
	}

	// exit boot service
	ret = ExitBootService(image_handle, &memmap);
	if (EFI_ERROR(ret))
	{
		Print(L"Failed to ExitBootService: %r\n", ret);
		goto out;
	}

	// launch kernel
	UINT64 entry_addr = *(UINT64 *)(kernel_base_addr + 24);

	typedef void EntryPointType(void);
	EntryPointType *entry_point = (EntryPointType *)entry_addr;
	entry_point();

	Print(L"All done\n");

out:
	while (1)
		;
	return EFI_SUCCESS;
}
