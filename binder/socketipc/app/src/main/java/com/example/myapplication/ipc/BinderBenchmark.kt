package com.example.myapplication.ipc

import android.util.Log
import com.example.myapplication.ICalculator
import java.util.concurrent.Callable
import java.util.concurrent.Executors
import java.util.concurrent.TimeUnit
import java.util.concurrent.atomic.AtomicInteger
import kotlin.system.measureNanoTime

data class BenchmarkStats(
    val minMs: Double,
    val maxMs: Double,
    val avgMs: Double,
    val p95Ms: Double,
    val success: Int,
    val failure: Int,
) {
    fun format(): String =
        "success=$success, failure=$failure, min=${"%.3f".format(minMs)}ms, " +
            "avg=${"%.3f".format(avgMs)}ms, p95=${"%.3f".format(p95Ms)}ms, " +
            "max=${"%.3f".format(maxMs)}ms"

    companion object {
        fun from(latencies: List<Double>, success: Int, failure: Int): BenchmarkStats {
            if (latencies.isEmpty()) {
                return BenchmarkStats(0.0, 0.0, 0.0, 0.0, success, failure)
            }
            val sorted = latencies.sorted()
            val avg = sorted.average()
            val p95Index = ((sorted.size - 1) * 0.95).toInt().coerceAtMost(sorted.lastIndex)
            return BenchmarkStats(
                minMs = sorted.first(),
                maxMs = sorted.last(),
                avgMs = avg,
                p95Ms = sorted[p95Index],
                success = success,
                failure = failure,
            )
        }
    }
}

object BinderBenchmark {

    private const val TAG = "BinderBenchmark"

    fun runAdd(calculator: ICalculator, threadCount: Int, iterations: Int): BenchmarkStats {
        Log.d(TAG, "[Flow:BenchBinder] runAdd: threads=$threadCount, iterations=$iterations")
        return runParallel(threadCount, iterations) { index ->
            calculator.add(index, index + 1)
        }
    }

    fun runCustom(
        calculator: ICalculator,
        payload: ByteArray,
        threadCount: Int,
        iterations: Int,
    ): BenchmarkStats {
        Log.d(TAG, "[Flow:BenchBinder] runCustom: threads=$threadCount, iterations=$iterations")
        return runParallel(threadCount, iterations) {
            val result = calculator.processCustomPayload(payload)
            check(NativeIpc.nativeDescribePayload(result, false).contains("processed"))
        }
    }

    fun runParcel(
        calculator: ICalculator,
        payload: ByteArray,
        threadCount: Int,
        iterations: Int,
    ): BenchmarkStats {
        Log.d(TAG, "[Flow:BenchBinder] runParcel: threads=$threadCount, iterations=$iterations")
        return runParallel(threadCount, iterations) {
            val result = calculator.processParcelPayload(payload)
            check(NativeIpc.nativeDescribePayload(result, true).contains("processed"))
        }
    }

    private fun runParallel(
        threadCount: Int,
        iterations: Int,
        block: (Int) -> Unit,
    ): BenchmarkStats {
        val pool = Executors.newFixedThreadPool(threadCount)
        val latencies = mutableListOf<Double>()
        val success = AtomicInteger(0)
        val failure = AtomicInteger(0)

        val tasks = (0 until threadCount).map {
            Callable {
                repeat(iterations) { index ->
                    val elapsedMs = measureNanoTime {
                        try {
                            block(index)
                            success.incrementAndGet()
                        } catch (_: Exception) {
                            failure.incrementAndGet()
                        }
                    } / 1_000_000.0
                    synchronized(latencies) {
                        latencies.add(elapsedMs)
                    }
                }
            }
        }

        Log.d(TAG, "[Flow:BenchBinder] runParallel 开始: threads=$threadCount, iterations=$iterations")
        pool.invokeAll(tasks)
        pool.shutdown()
        pool.awaitTermination(5, TimeUnit.MINUTES)
        val stats = BenchmarkStats.from(latencies, success.get(), failure.get())
        Log.d(TAG, "[Flow:BenchBinder] runParallel 完成: ${stats.format()}")
        return stats
    }
}
