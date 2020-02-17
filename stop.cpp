#include "stop.hpp"

#include <csignal>
#include <stdexcept>

void tloRequestStop(int signal) {
  static_cast<void>(signal);

  tlo::stopRequested = true;
}

namespace tlo {
std::atomic<bool> stopRequested(false);

void registerInterruptSignalHandler(TloSignalHandler signalHandler) {
  auto oldSignalHandler = std::signal(SIGINT, signalHandler);

  if (oldSignalHandler == SIG_ERR) {
    throw std::runtime_error("Error: Failed to register signal handler.");
  }
}
}  // namespace tlo
