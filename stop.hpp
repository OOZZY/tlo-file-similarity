#ifndef TLOFS_STOP_HPP
#define TLOFS_STOP_HPP

#include <atomic>

extern "C" {
using TloSignalHandler = void (*)(int);
void tloRequestStop(int signal);
}

namespace tlo {
extern std::atomic<bool> stopRequested;

// Throws std::runtime_error if registration fails.
void registerInterruptSignalHandler(TloSignalHandler signalHandler);
}  // namespace tlo

#endif  // TLOFS_STOP_HPP
