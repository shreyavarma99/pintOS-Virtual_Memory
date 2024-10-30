#include "userprog/syscall.h"
#include "lib/user/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "userprog/pagedir.h"
#include "threads/vaddr.h"
#include "threads/synch.h"
#include "devices/shutdown.h"
#include "filesys/file.h"
#include "filesys/filesys.h"

static void syscall_handler (struct intr_frame *);
void is_valid (void *uaddr);
int write (int fd, const void *buffer, unsigned size);
void exit (int status);
void check_args (void *esp, int num);
struct lock file_mutex;
unsigned tell (int fd);
void buf_valid (const void *buffer, unsigned size);
void string_valid (const char* str);

void syscall_init (void)
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
  lock_init (&file_mutex);
}

static void syscall_handler (struct intr_frame *f UNUSED)
{
  /* Shreya Agrawal, Shreya Varma, Garv, Jyotsna driving */

  /* check all 4 bytes of the pointer */
  is_valid ((char *) f->esp);

  /* get arguments */
  int system_call = *(int *) f->esp;
  int *one = (int *) f->esp + 1;
  int *two = (int *) f->esp + 2;
  int *three = (int *) f->esp + 3;

  switch (system_call)
    {
      case (SYS_HALT):
        halt ();
        break;
      case (SYS_EXIT):
        check_args (f->esp, 1);
        exit (*one);
        break;
      case (SYS_EXEC):  
        check_args (f->esp, 1); 
        string_valid ((char *) *one);      
        f->eax = exec ((char *) *one);
        break;
      case (SYS_WAIT):
        check_args (f->esp, 1);
        f->eax = wait ((pid_t) *one);
        break;
      case (SYS_CREATE):
        check_args (f->esp, 2);
        f->eax = create ((char *) *one, (unsigned) *two);
        break;
      case (SYS_REMOVE):
        check_args (f->esp, 1);
        f->eax = remove ((char *) *one);
        break;
      case (SYS_OPEN):
        check_args (f->esp, 1);
        f->eax = open ((char *) *one);
        break;
      case (SYS_FILESIZE):
        check_args (f->esp, 1);
        f->eax = filesize (*one);
        break;
      case (SYS_READ):
        check_args (f->esp, 3);
        f->eax = read (*one, (void *) *two, (unsigned) *three);
        break;
      case (SYS_WRITE):
        check_args (f->esp, 3);
        buf_valid ((void *) *two, (unsigned) *three);
        f->eax = write (*one, (void *) *two, (unsigned) *three);
        break;
      case (SYS_SEEK):
        check_args (f->esp, 2);
        seek (*one, (unsigned) *two);
        break;
      case (SYS_TELL):
        check_args (f->esp, 1);
        f->eax = tell (*one);
        break;
      case (SYS_CLOSE):
        check_args (f->esp, 1);
        close (*one);
        break;
      default:
        break;
    }
}

/**
 * Terminates PINTOS.
 */
void halt () 
{ 
  /* Shreya V. driving */
  shutdown_power_off (); 
}

/**
 * Exit system call
 * @param status: status to be exited with
 * deallocate memory for successful termination
 * ensure that all of process's children exit successfully
 * note: process cannot exit unless its parent thread calls wait
 * tell parent that it is allowed to finish wait
 */
void exit (int status)
{
  /* Shreya V. and Jyotsna driving */
  struct thread *current = thread_current ();
  lock_acquire (&file_mutex);
  
  if (current->executable) 
    {
      file_allow_write (current->executable);
      file_close (current->executable);
    }
  lock_release (&file_mutex);

  /* close all open files */
  for (int i = 0; i < 128; i++)
    {
      close (i);
    }
  
  /* wait for parent to call wait on this child */
  sema_down (&current->zombie);
  if (current->parent)
    {
      current->parent->child_exit_status = status;
    }
  printf ("%s: exit(%d)\n", current->name, status);

  /* iterate through children's list and allow exit */
  struct list_elem *e;
  for (e = list_begin (&current->children); 
        e != list_end (&current->children); e = list_next (e))
    {
      struct thread *temp = list_entry (e, struct thread, child_elem);
      sema_up (&temp->zombie);  
    }
  
  list_remove (&current->child_elem);
  /* tell parent it is allowed to complete its wait */
  sema_up (&current->parent->wait);

  thread_exit ();
}

/**
 * Starts a new process running a user program loaded from FILENAME
 * @param cmd_line: passed in command line
 * @return pid of new process
 */
pid_t exec (const char *cmd_line)
{
  /* Garv and Shreya V. driving */
  /* validate command line */
  string_valid (cmd_line);

  // lock_acquire (&file_mutex);
  /* execute */
  tid_t result = process_execute (cmd_line);
  // lock_release (&file_mutex);

  return result;
}

/**
 * Waits for process PID to die and returns its exit status.
 * @param pid: pid of process to wait on
 * @return exit status
 */
int wait (pid_t pid)
{
  /* Shreya A. driving */
  return process_wait (pid);
}

/**
 * Creates a new file called file initially initial_size bytes in size. 
 * Returns true if successful, false otherwise.
 * @param file: file name to create
 * @param initial_size size
 */
bool create (const char *file, unsigned initial_size)
{
  /* Jyotsna driving */
  /* validate file name */
  string_valid (file);

  lock_acquire (&file_mutex);
  /* create file */
  bool result = filesys_create (file, initial_size);
  lock_release (&file_mutex);
  
  return result;
}

/**
 * Deletes the file called file. Returns true if successful, false otherwise.
 * @param file: file to delete
 */
bool remove (const char *file) 
{
  /* Jyotsna driving */
  /* validate file name */
  string_valid (file);
  lock_acquire (&file_mutex);
  bool result = filesys_remove (file);
  lock_release (&file_mutex);
  return result;
}

/**
 * Opens the file called file. Returns a nonnegative integer handle called a 
 * "file descriptor" (fd) or -1 if the file could not be opened.
 * @param file to open
 */
int open (const char* file)
{
  /* Shreya A., Jyotsna driving */
  string_valid (file);

  /* ensure open is atomic */
  lock_acquire (&file_mutex);
  struct file *currentFile = filesys_open (file);
  lock_release (&file_mutex);
  struct thread *current = thread_current ();
 
  if (currentFile == NULL)
    {
      /* file was not opened successfully */
      return -1;
    }
  if (current->current_fd == 127)
  {
    for (int i = 0; i < 128; i++)
    {
      if (current->files[i] == NULL)
      {
        current->files[i] = currentFile;
        return current->current_fd;
      }
    }
  }
  else
  {
    current->current_fd += 1;
    current->files[current->current_fd] = currentFile;
  }
  return current->current_fd;
}

/**
 * Returns the size, in bytes, of the file open as fd.
 * @param fd: given file
 */
int filesize (int fd)
{
  /* Shreya A., Jyotsna driving */
  /* ensure fd is a valid number for file descriptor */
  if (fd <= 1 || fd >= 128) 
    {
      exit (-1);
    }
    
  if (!thread_current ()->files[fd]) 
    {
      /* if proccess does not currently have file open */
      return 0;
    }

  lock_acquire (&file_mutex);
  int result = file_length (thread_current ()->files[fd]);
  lock_release (&file_mutex);
  return result;
}

/**
 * Reads size bytes from the file open as fd into buffer.
 * Returns the number of bytes actually read (0 at end of file), 
 * or -1 if the file could not be read.
 * @param fd: file to read from
 * @param buffer: buffer to read into
 * @param size: number of bytes to read 
 */
int read (int fd, void *buffer, unsigned size) 
{
  /* Jyotsna, Garv, Shreya A. driving */
  buf_valid (buffer, size);

  struct thread* current = thread_current ();

  /* valid fd */
  if (fd < 0 || fd >= 128)
    {
      exit (-1);
    }

  /* reading from stdin */
  if (fd == STDIN_FILENO)
    {
      uint8_t *temp_buff = (uint8_t *) buffer;
      for (int i = 0; i < (int) size; i++)
        {
          temp_buff[i] = input_getc ();
        }
      return size;
    }

  /* check file valid */
  if (current->files[fd] == NULL)
    {
      exit (-1);
    }
  else 
    {
      /* read from file */
      lock_acquire (&file_mutex);
      int result = file_read (current->files[fd], buffer, size);
      lock_release (&file_mutex);
      return result;
    }
}


/**
 * Writes size bytes from buffer to the open file fd. Returns the number 
 * of bytes actually written, which may be less than size if some 
 * bytes could not be written.
 * @param fd: file to write to
 * @param buffer: buffer to
 * @param size: number of bytes to read
 */
int write (int fd, const void *buffer, unsigned size)
{
  /* Garv, Shreya V., and Jyotsna Driving */
  if (fd < 0 || fd >= 128)
    {
      exit (-1);
    }

  /* if writing to console */
  if (fd == STDOUT_FILENO)
    {
      int remainingSize = size;

      /* write output to console in pieces so that it fits */
      while (remainingSize > 512)
        {
          putbuf (buffer, 512);
          remainingSize -= 512;
        }

      putbuf (buffer, remainingSize);
      return size;
    }
  else
    {
      /* writing to file */
      struct thread *current = thread_current ();
      if (current->files[fd] == NULL)
        {
          /* proc does not have file opened */
          exit (-1);
        }
      
      /* write to file atomically */
      lock_acquire (&file_mutex);
      int written_b = file_write (current->files[fd], buffer, size);
      lock_release (&file_mutex);
      return written_b;
    }
}

/**
 * Changes the next byte to be read or written in open file fd to position, 
 * expressed in bytes from the beginning of the file.
 * @param fd: given file
 * @param position: position of next
 */
void seek (int fd, unsigned position)
{
  /* Garv Driving */
  /* make sure fd is valid and opened for this thread */
  if (fd <= 1 || fd >= 128 || !thread_current ()->files[fd]) 
    {
      exit (-1);
    }
  lock_acquire (&file_mutex);
  /* file seek */
  file_seek (thread_current ()->files[fd], (off_t) position);
  lock_release (&file_mutex);
}

/**
 * Returns the position of the next byte to be read or written in open file fd, 
 * expressed in bytes from the beginning of the file.
 * @param fd: given file
 */
unsigned tell (int fd)
{
  /* Jyotsna and Shreya V. driving */

  /* make sure fd is valid and opened for this thread */
  if (fd <= 1 || fd >= 128 || !thread_current ()->files[fd])
    {
      exit (-1);
    }
  else
    {
      /* ensure tell operation is atomic */
      lock_acquire (&file_mutex);
      off_t ret = file_tell (thread_current ()->files[fd]);
      lock_release (&file_mutex);
      return (unsigned) ret;
    }
}

/**
 * Closes file descriptor fd.
 * @param fd: file descriptor of file to close
 */
void close (int fd)
{
  /* Shreya A. and Garv driving */
  if (fd < 0 || fd >= 128)
    {
      exit (-1);
    }
  
  /* if process has file open, close file */
  if (thread_current ()->files[fd]) 
    {
      lock_acquire (&file_mutex);
      file_close (thread_current ()->files[fd]);
      lock_release (&file_mutex);
      thread_current ()->files[fd] = NULL;
      if (fd == thread_current ()->current_fd)
        {
          thread_current ()->current_fd -= 1;
        }
    }
}

/**
 * Method to validate whether argument addresses are valid
 * esp is the first pointer value to begin with, 
 * and num is the number of arguments we are checking
 */
void check_args (void *esp, int num)
{
  /* Jyotsna driving */
  int *cur = (int *) esp + 1;
  for (int i = 0; i < num; i++)
    {
      /* check if this argument is valid */
      is_valid (cur);
      cur = (int *) cur + 1;
    }
}

/**
 * Validate an address, taking in an address and 
 * checking if each byte is not null, is part of user memory, and is mapped
 */
void is_valid (void *uaddr)
{
  /* Garv, Shreya V., and Jyotsna driving */
  /* check every byte of the address */
  for (int j = 0; j < 4; j++) 
    {
      /* check all three conditions for that byte */
      if ((char *) uaddr + j == NULL || !is_user_vaddr ((char *) uaddr + j) ||
        !pagedir_get_page (thread_current ()->pagedir, (char *) uaddr + j))
        {
          exit (-1);
        }
    }
}

/**
 * Check if a buffer contains valid addresses
 * Takes in the buffer and the size we want to check
 * and checks if the buffer is valid by iterating by PGSIZE 
 */
void buf_valid (const void *buffer, unsigned size) 
{
  /* Garv and Shreya V. driving */
  char *temporary_buffer = (char *) buffer;
  char *end_of_buffer = temporary_buffer + size;

  /* check byte on each page for validity */
  while (temporary_buffer < end_of_buffer)
    {
      is_valid ((void *) temporary_buffer);
      temporary_buffer += PGSIZE;
    }
  
  is_valid((void *) end_of_buffer);
}

/* 
 * Check if a given string is valid.
 */
void string_valid (const char* str)
{
    if (str == NULL)
      {
        exit (-1);
      }

    /* Check every byte of the string, including the null terminator */
    while (true)
      {
        /* Check if the current character pointer is valid */

        if ((char *) str == NULL || !is_user_vaddr ((char *) str) ||
        !pagedir_get_page (thread_current ()->pagedir, (char *) str))
        {
          exit (-1);
        }

        /* If we reach the null terminator, stop the loop */
        if (*str == '\0')
          {
            break;
          }

        /* Move to the next byte in the string */
        str = (char *) str + 1;
      }
}