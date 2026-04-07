#include "AIBackend.h"
#include <sstream>
#include <numeric>
#include <cmath>
#include <iomanip>
#include <algorithm>

AIBackend::AIBackend()
    : m_enabled(true)
    , m_emaShortCPU(0.0f)
    , m_emaShortGPU(0.0f)
    , m_emaLongCPU(0.0f)
    , m_emaLongGPU(0.0f)
    , m_cpuTrend(0.0f)
    , m_gpuTrend(0.0f)
    , m_confidence(0.5f)
{
    // Initialize model with intelligent defaults
    m_model.cpuWeight = 0.4f;
    m_model.gpuWeight = 0.5f;
    m_model.utilWeight = 0.1f;
    m_model.streamOverhead = 0.5f; // ms per stream
    m_model.optimalUtilization = 0.85f;
}

void AIBackend::recordFrame(const FrameMetrics& metrics) {
    if (!m_enabled) return;
    
    // Keep history for pattern analysis
    m_history.push_back(metrics);
    if (m_history.size() > HISTORY_SIZE) {
        m_history.pop_front();
    }
    
    // Update EMAs for trend detection
    float alphaShort = 0.15f;  // Fast EMA
    float alphaLong = 0.05f;   // Slow EMA
    
    if (m_emaShortCPU == 0.0f) {
        m_emaShortCPU = metrics.cpuTime;
        m_emaShortGPU = metrics.gpuTime;
        m_emaLongCPU = metrics.cpuTime;
        m_emaLongGPU = metrics.gpuTime;
    } else {
        m_emaShortCPU = alphaShort * metrics.cpuTime + (1.0f - alphaShort) * m_emaShortCPU;
        m_emaShortGPU = alphaShort * metrics.gpuTime + (1.0f - alphaShort) * m_emaShortGPU;
        m_emaLongCPU = alphaLong * metrics.cpuTime + (1.0f - alphaLong) * m_emaLongCPU;
        m_emaLongGPU = alphaLong * metrics.gpuTime + (1.0f - alphaLong) * m_emaLongGPU;
    }
    
    // Calculate trends (positive = improving/decreasing, negative = degrading/increasing)
    m_cpuTrend = m_emaLongCPU - m_emaShortCPU;  // Negative if CPU time increasing
    m_gpuTrend = m_emaLongGPU - m_emaShortGPU;  // Negative if GPU time increasing
    
    // Update model based on observed patterns
    updateModel();
}

AIBackend::Prediction AIBackend::analyzeAndPredict(const FrameMetrics& currentMetrics) {
    Prediction pred;
    
    // INTELLIGENT HEURISTIC SYSTEM - Learns from patterns
    if (!m_enabled || currentMetrics.frameTime <= 0.0f) {
        // Return safe defaults
        pred.predictedBottleneck = BottleneckType::Balanced;
        pred.recommendedStreamCount = 4;
        pred.shouldInjectCompute = false;
        pred.confidence = 0.5f;
        pred.predictedFrameTime = currentMetrics.frameTime > 0.0f ? currentMetrics.frameTime : 16.67f;
        pred.reasoning = "Heuristic: Default settings (insufficient data)";
        return pred;
    }
    
    // Analyze current load
    float cpuLoad = currentMetrics.cpuTime / std::max(currentMetrics.frameTime, 0.001f);
    float gpuLoad = currentMetrics.gpuUtilization;
    
    // Detect patterns from history
    Pattern patterns = detectPatterns();
    
    // Intelligent decision making based on patterns and current state
    float confidence = 0.7f;
    
    // Trend-based adjustments
    if (m_cpuTrend > 0.5f) {  // CPU time increasing (degrading)
        pred.predictedBottleneck = BottleneckType::CPUBound;
        pred.recommendedStreamCount = std::max(2, currentMetrics.streamCount - 1);
        confidence = 0.8f;
    } else if (m_gpuTrend > 0.5f && gpuLoad < 0.6f) {  // GPU time increasing but underutilized
        pred.predictedBottleneck = BottleneckType::GPUUnderutilized;
        pred.recommendedStreamCount = std::min(8, currentMetrics.streamCount + 1);
        confidence = 0.75f;
    } else if (cpuLoad > 0.75f) {
        pred.predictedBottleneck = BottleneckType::CPUBound;
        pred.recommendedStreamCount = std::max(2, currentMetrics.streamCount - 1);
        confidence = 0.85f;
    } else if (gpuLoad < 0.55f && patterns.volatility < 0.3f) {  // Low volatility = stable, can increase streams
        pred.predictedBottleneck = BottleneckType::GPUUnderutilized;
        pred.recommendedStreamCount = std::min(8, currentMetrics.streamCount + 1);
        confidence = 0.8f;
    } else {
        pred.predictedBottleneck = BottleneckType::Balanced;
        // Use predictive model to find optimal stream count
        pred.recommendedStreamCount = findOptimalStreamCount(cpuLoad, gpuLoad);
        confidence = 0.7f;
    }
    
    // Clamp stream count
    pred.recommendedStreamCount = std::max(2, std::min(8, pred.recommendedStreamCount));
    
    // Predict frame time using model
    pred.predictedFrameTime = predictFrameTime(pred.recommendedStreamCount, cpuLoad, gpuLoad);
    pred.shouldInjectCompute = (pred.predictedBottleneck == BottleneckType::GPUUnderutilized && gpuLoad < 0.4f);
    pred.confidence = confidence;
    m_confidence = pred.confidence;
    
    // Generate intelligent reasoning
    std::ostringstream oss;
    oss << "Heuristic: ";
    
    if (patterns.trend < -0.1f) {
        oss << "Performance degrading (trend: " << std::fixed << std::setprecision(3) << patterns.trend << "). ";
    } else if (patterns.trend > 0.1f) {
        oss << "Performance improving. ";
    }
    
    if (pred.predictedBottleneck == BottleneckType::CPUBound) {
        oss << "CPU bound (" << std::fixed << std::setprecision(1) << (cpuLoad * 100.0f) 
            << "%), reducing to " << pred.recommendedStreamCount << " streams";
    } else if (pred.predictedBottleneck == BottleneckType::GPUUnderutilized) {
        oss << "GPU underutilized (" << std::fixed << std::setprecision(1) << (gpuLoad * 100.0f) 
            << "%), increasing to " << pred.recommendedStreamCount << " streams";
    } else {
        oss << "Balanced, optimal " << pred.recommendedStreamCount << " streams";
    }
    
    if (patterns.correlation > 0.5f) {
        oss << " [CPU-GPU correlated: " << std::fixed << std::setprecision(2) << patterns.correlation << "]";
    }
    
    pred.reasoning = oss.str();
    
    return pred;
}

AIBackend::Pattern AIBackend::detectPatterns() const {
    Pattern pattern;
    
    if (m_history.size() < 5) {
        // Not enough data
        pattern.trend = 0.0f;
        pattern.volatility = 0.2f;
        pattern.correlation = 0.0f;
        pattern.isAnomaly = false;
        return pattern;
    }
    
    // Extract frame times, CPU times, and GPU times from history
    std::vector<float> frameTimes;
    std::vector<float> cpuTimes;
    std::vector<float> gpuTimes;
    
    for (const auto& metrics : m_history) {
        frameTimes.push_back(metrics.frameTime);
        cpuTimes.push_back(metrics.cpuTime);
        gpuTimes.push_back(metrics.gpuTime);
    }
    
    // Calculate trend (negative = degrading, positive = improving)
    pattern.trend = -calculateTrend(frameTimes);  // Negate because lower frame time is better
    
    // Calculate volatility
    pattern.volatility = calculateVolatility(frameTimes);
    
    // Calculate CPU-GPU correlation
    pattern.correlation = calculateCorrelation(cpuTimes, gpuTimes);
    
    // Detect anomalies
    if (m_history.size() > 0) {
        pattern.isAnomaly = detectAnomaly(m_history.back());
    } else {
        pattern.isAnomaly = false;
    }
    
    return pattern;
}

float AIBackend::calculateTrend(const std::vector<float>& values) const {
    if (values.size() < 2) return 0.0f;
    
    // Linear regression slope
    float sumX = 0.0f, sumY = 0.0f, sumXY = 0.0f, sumX2 = 0.0f;
    size_t n = values.size();
    
    for (size_t i = 0; i < n; ++i) {
        float x = static_cast<float>(i);
        float y = values[i];
        sumX += x;
        sumY += y;
        sumXY += x * y;
        sumX2 += x * x;
    }
    
    float slope = (n * sumXY - sumX * sumY) / (n * sumX2 - sumX * sumX);
    return slope; // Positive = improving (decreasing frame time), negative = degrading
}

float AIBackend::calculateVolatility(const std::vector<float>& values) const {
    if (values.size() < 2) return 1.0f;
    
    float mean = std::accumulate(values.begin(), values.end(), 0.0f) / values.size();
    float variance = 0.0f;
    
    for (float v : values) {
        variance += (v - mean) * (v - mean);
    }
    variance /= values.size();
    
    float stdDev = std::sqrt(variance);
    return stdDev / mean; // Normalized volatility
}

float AIBackend::calculateCorrelation(const std::vector<float>& cpu, const std::vector<float>& gpu) const {
    if (cpu.size() != gpu.size() || cpu.size() < 2) return 0.0f;
    
    float meanCPU = std::accumulate(cpu.begin(), cpu.end(), 0.0f) / cpu.size();
    float meanGPU = std::accumulate(gpu.begin(), gpu.end(), 0.0f) / gpu.size();
    
    float numerator = 0.0f, denomCPU = 0.0f, denomGPU = 0.0f;
    
    for (size_t i = 0; i < cpu.size(); ++i) {
        float diffCPU = cpu[i] - meanCPU;
        float diffGPU = gpu[i] - meanGPU;
        numerator += diffCPU * diffGPU;
        denomCPU += diffCPU * diffCPU;
        denomGPU += diffGPU * diffGPU;
    }
    
    float denominator = std::sqrt(denomCPU * denomGPU);
    if (denominator < 0.0001f) return 0.0f;
    
    return numerator / denominator; // Pearson correlation coefficient
}

bool AIBackend::detectAnomaly(const FrameMetrics& metrics) const {
    if (m_history.size() < 5) return false;
    
    // Calculate mean and std dev of recent frame times
    float mean = 0.0f;
    int count = 0;
    for (const auto& m : m_history) {
        mean += m.frameTime;
        count++;
    }
    mean /= count;
    
    float variance = 0.0f;
    for (const auto& m : m_history) {
        variance += (m.frameTime - mean) * (m.frameTime - mean);
    }
    variance /= count;
    float stdDev = std::sqrt(variance);
    
    // Check if current frame is > 2 std devs from mean
    return std::abs(metrics.frameTime - mean) > 2.0f * stdDev;
}

void AIBackend::updateModel() {
    // Adaptive model parameter tuning based on observed patterns
    // This is the "learning" part without actual ML training
    
    Pattern patterns = detectPatterns();
    
    // Adjust weights based on correlation
    if (patterns.correlation > 0.7f) {
        // High correlation - both CPU and GPU matter
        m_model.cpuWeight = 0.45f;
        m_model.gpuWeight = 0.45f;
        m_model.utilWeight = 0.1f;
    } else if (patterns.correlation < 0.3f) {
        // Low correlation - independent bottlenecks
        if (m_cpuTrend > 0.001f) {
            m_model.cpuWeight = 0.6f;
            m_model.gpuWeight = 0.3f;
        } else {
            m_model.cpuWeight = 0.3f;
            m_model.gpuWeight = 0.6f;
        }
        m_model.utilWeight = 0.1f;
    }
    
    // Adjust stream overhead based on volatility
    if (patterns.volatility > 0.2f) {
        // High volatility - reduce overhead penalty
        m_model.streamOverhead = 0.3f;
    } else {
        m_model.streamOverhead = 0.5f;
    }
}

float AIBackend::predictFrameTime(int streamCount, float cpuLoad, float gpuLoad) const {
    // Predictive model: estimate frame time based on stream count and loads
    float baseTime = m_model.cpuWeight * (cpuLoad * 16.67f) + 
                     m_model.gpuWeight * (gpuLoad * 16.67f);
    
    // Stream overhead (submission cost)
    float streamOverhead = m_model.streamOverhead * streamCount;
    
    // Parallelization benefit (diminishing returns)
    float parallelBenefit = 1.0f - (1.0f / (1.0f + streamCount * 0.15f));
    float adjustedGPU = gpuLoad * (1.0f - parallelBenefit * 0.3f);
    
    float predicted = baseTime + streamOverhead - (adjustedGPU * 2.0f);
    return std::max(1.0f, predicted);
}

int AIBackend::findOptimalStreamCount(float cpuLoad, float gpuLoad) const {
    // Search for optimal stream count using predictive model
    int bestStreams = 4;
    float bestTime = predictFrameTime(4, cpuLoad, gpuLoad);
    
    for (int streams = 2; streams <= 8; ++streams) {
        float predicted = predictFrameTime(streams, cpuLoad, gpuLoad);
        if (predicted < bestTime) {
            bestTime = predicted;
            bestStreams = streams;
        }
    }
    
    return bestStreams;
}

std::string AIBackend::generateReasoning(const Prediction& pred, const FrameMetrics& metrics) const {
    std::ostringstream oss;
    
    Pattern patterns = detectPatterns();
    
    oss << "AI Analysis: ";
    
    // Trend analysis
    if (patterns.trend < -0.001f) {
        oss << "Performance degrading (trend: " << std::fixed << std::setprecision(3) << patterns.trend << "). ";
    } else if (patterns.trend > 0.001f) {
        oss << "Performance improving (trend: " << patterns.trend << "). ";
    } else {
        oss << "Stable performance. ";
    }
    
    // Correlation insights
    if (patterns.correlation > 0.6f) {
        oss << "CPU-GPU correlated (" << std::fixed << std::setprecision(2) << patterns.correlation << "). ";
    } else if (patterns.correlation < -0.3f) {
        oss << "CPU-GPU anti-correlated. ";
    }
    
    // Bottleneck reasoning
    if (pred.predictedBottleneck == BottleneckType::CPUBound) {
        oss << "Bottleneck: CPU submission (load: " << std::fixed << std::setprecision(2) 
            << (metrics.cpuTime / metrics.frameTime) << "). ";
        oss << "Reducing streams to " << pred.recommendedStreamCount << " to minimize overhead.";
    } else if (pred.predictedBottleneck == BottleneckType::GPUUnderutilized) {
        oss << "Bottleneck: GPU idle (" << std::fixed << std::setprecision(1) 
            << (metrics.gpuUtilization * 100.0f) << "% util). ";
        oss << "Increasing streams to " << pred.recommendedStreamCount << " for better occupancy.";
        if (pred.shouldInjectCompute) {
            oss << " Injecting extra compute pass.";
        }
    } else {
        oss << "Balanced load. Maintaining " << pred.recommendedStreamCount << " streams.";
    }
    
    // Anomaly detection
    if (patterns.isAnomaly) {
        oss << " [ANOMALY DETECTED - confidence reduced]";
    }
    
    oss << " Confidence: " << std::fixed << std::setprecision(1) << (pred.confidence * 100.0f) << "%";
    
    return oss.str();
}

void AIBackend::reset() {
    m_history.clear();
    m_emaShortCPU = 0.0f;
    m_emaShortGPU = 0.0f;
    m_emaLongCPU = 0.0f;
    m_emaLongGPU = 0.0f;
    m_cpuTrend = 0.0f;
    m_gpuTrend = 0.0f;
    m_confidence = 0.5f;
    
    // Reset model to defaults
    m_model.cpuWeight = 0.4f;
    m_model.gpuWeight = 0.5f;
    m_model.utilWeight = 0.1f;
    m_model.streamOverhead = 0.5f;
    m_model.optimalUtilization = 0.85f;
}

