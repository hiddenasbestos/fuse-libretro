// Compatibility file functions

#include <libretro.h>
#include <externs.h>
#include <compat.h>
#include <ui/ui.h>
#include <utils.h>

#include <src/compat/48.rom.h>
#include <src/compat/tape_48.szx.h>

typedef struct
{
   const char *name;
   const unsigned char* ptr;
   size_t size;
}
entry_t;

static const entry_t mem_entries[] = {
   {
      "48.rom",
      fuse_roms_48_rom, sizeof(fuse_roms_48_rom)
   },
   {
      "tape_48.szx",
      fuse_lib_uncompressed_tape_48_szx, sizeof(fuse_lib_uncompressed_tape_48_szx)
   }
};

static const entry_t* find_entry(const char *path)
{
   static entry_t tape;
   
   int i;
   size_t len = strlen(path);
   
   // * signals us to load the tape data
   if (path[0] == '*')
   {
      tape.name = NULL;
      tape.ptr = (const unsigned char*)tape_data;
      tape.size = tape_size;
      return &tape;
   }
   
   for (i = 0; i < sizeof(mem_entries) / sizeof(mem_entries[0]); i++)
   {
      size_t len2 = strlen(mem_entries[i].name);
      
      if (!strcmp(path + len - len2, mem_entries[i].name))
      {
         return mem_entries + i;
      }
   }
   
   return NULL;
}

typedef struct
{
   const char* ptr;
   size_t length, remain;
}
compat_fd_internal;

const compat_fd COMPAT_FILE_OPEN_FAILED = NULL;

compat_fd compat_file_open(const char *path, int write)
{
   if (!write)
   {
      const entry_t* entry = find_entry(path);
      
      if (entry != NULL)
      {
         compat_fd_internal *fd = (compat_fd_internal*)malloc(sizeof(compat_fd_internal));
         
         if (fd)
         {
            fd->ptr = entry->ptr;
            fd->length = fd->remain = entry->size;
            
            log_cb(RETRO_LOG_INFO, "Opened \"%s\" from memory\n", path);
            return (compat_fd)fd;
         }
         else
         {
            log_cb(RETRO_LOG_ERROR, "Out of memory\n");
         }
      }
      else
      {
         log_cb(RETRO_LOG_ERROR, "Could not find file %s\n", path);
      }
   }
   else
   {
      log_cb(RETRO_LOG_ERROR, "Cannot open %s for writing\n", path);
   }
   
   return COMPAT_FILE_OPEN_FAILED;
}

off_t compat_file_get_length(compat_fd cfd)
{
   compat_fd_internal *fd = (compat_fd_internal*)cfd;
   return (off_t)fd->length;
}

int compat_file_read(compat_fd cfd, utils_file *file)
{
   compat_fd_internal *fd = (compat_fd_internal*)cfd;
   size_t numread;
   
   numread = file->length < fd->remain ? file->length : fd->remain;
   memcpy(file->buffer, fd->ptr, numread);
   fd->ptr += numread;
   fd->remain -= numread;
   
   if (numread == file->length)
   {
      return 0;
   }
   
   ui_error( UI_ERROR_ERROR,
             "error reading file: expected %lu bytes, but read only %lu",
             (unsigned long)file->length, (unsigned long)numread );
   return 1;
}

int compat_file_write(compat_fd cfd, const unsigned char *buffer, size_t length)
{
   (void)cfd;
   (void)buffer;
   (void)length;
   return 1;
}

int compat_file_close(compat_fd cfd)
{
   compat_fd_internal *fd = (compat_fd_internal*)cfd;
   free(fd);
   return 0;
}

int compat_file_exists(const char *path)
{
   log_cb(RETRO_LOG_INFO, "Checking if \"%s\" exists\n", path);
   return find_entry(path) != NULL;
}
