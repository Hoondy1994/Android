#pragma once

#include "ipc/types.hpp"

namespace ipc {

enum class SerializationKind {
    kCustom,
    kParcel,
};

enum class SocketBenchmarkMode {
    kAdd,
    kCustom,
    kParcel,
};

[[nodiscard]] BenchmarkStats run_serialization_benchmark(SerializationKind kind, int thread_count,
                                                         int iterations);

[[nodiscard]] BenchmarkStats run_socket_benchmark(SocketBenchmarkMode mode, int thread_count,
                                                  int iterations);

}  // namespace ipc
