#pragma once
// Must link ntdll

#ifdef WIN32

// #include <windows.h>
#include <winternl.h>

extern NTSYSAPI NTSTATUS NTAPI NtSetTimerResolution(
	ULONG DesiredResolution,
	BOOLEAN SetResolution,
	PULONG CurrentResolution);

/* USAGE
 *
 * Request 100us timer resolution
 *
 * { ULONG time; NtSetTimerResolution(1000, TRUE, &time); }
 *
 * Reset timer resolution when you no longer need it. May not be necessary since Windows 10 update 2004, as this functionality in timeBeginPeriod was made program-local and this probably was as well.
 *
 * { ULONG time; NtSetTimerResolution(0, FALSE, &time); }
 *
 *    Requests a new interrupt timer resolution and returns the new value.
 *    The closest value that the host hardware can support is returned as the actual time.
 *
 * ARGUMENTS:
 *
 *    DesiredResolution - Supplies the desired time between timer interrupts in 100ns units.
 *    SetResoluion - A boolean value that determines whether the timer resolution should be set (TRUE) or reset (FALSE).
 *    CurrentResolution - Pointer to a variable that receives the actual resolution. If this is not provided, the new resolution will NOT be set.
 *
 * RETURN VALUE:
 *
 *    STATUS_SUCCESS is returned on success..
 *    STATUS_ACCESS_VIOLATION is returned if the output parameter for the actual time cannot be written into the CurrentResolution pointer.
*/

#endif