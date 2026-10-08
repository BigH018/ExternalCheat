#pragma once

// Process-wide flags shared with the console control handler. Windows runs that handler on a thread of its own, so
// these are the only cross-thread state in the tool (CLAUDE.md §6.2): the handler sets a flag, the main thread acts.

#include <atomic>

namespace core
{
// Ctrl+C, Ctrl+Break or the console window closing: the main loop finishes its frame and shuts down. app/frame also
// sets it when you ask to exit (exit key, Exit button, Alt+F4), so main() doesn't wait for Enter afterwards.
inline std::atomic<bool> shutdown_requested{false};
// Set by main() once the overlay is gone and the handle is closed, so the handler can let Windows end the process.
inline std::atomic<bool> shutdown_complete{false};
} // namespace core
