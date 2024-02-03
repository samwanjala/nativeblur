#include <jni.h>
#include <string>
#include <android/bitmap.h>
#include <android/log.h>

#define ALOGI(...) __android_log_print(ANDROID_LOG_INFO, "blurtask", __VA_ARGS__)

class BitmapGuard {
private:
    JNIEnv* env;
    jobject bitmap;
    AndroidBitmapInfo info;
    int bytesPerPixel;
    void* bytes;
    bool valid;

public:
    BitmapGuard(JNIEnv* env, jobject jBitmap) : env{env}, bitmap{jBitmap}, bytes{nullptr} {
        valid = false;
        if (AndroidBitmap_getInfo(env, bitmap, &info) != ANDROID_BITMAP_RESULT_SUCCESS) {
            return;
        }
        if (info.format != ANDROID_BITMAP_FORMAT_RGBA_8888 &&
            info.format != ANDROID_BITMAP_FORMAT_A_8) {
            return;
        }
        bytesPerPixel = info.stride / info.width;
        if (bytesPerPixel != 1 && bytesPerPixel != 4) {
            return;
        }
        if (AndroidBitmap_lockPixels(env, bitmap, &bytes) != ANDROID_BITMAP_RESULT_SUCCESS) {
            return;
        }
        valid = true;
    }
    ~BitmapGuard() {
        if (valid) {
            AndroidBitmap_unlockPixels(env, bitmap);
        }
    }
    uint8_t* get() const {
        return reinterpret_cast<uint8_t*>(bytes);
    }
    int width() const { return info.width; }
    int height() const { return info.height; }
    int vectorSize() const { return bytesPerPixel; }
};

void calculateGaussianWeights(float * kernel, int radius){
    memset(kernel, 0, sizeof(kernel));

    // Compute gaussian weights for the blur
    // e is the euler's number
    float e = 2.718281828459045f;
    float pi = 3.1415926535897932f;
    // g(x) = (1 / (sqrt(2 * pi) * sigma)) * e ^ (-x^2 / (2 * sigma^2))
    // x is of the form [-radius .. 0 .. radius]
    // and sigma varies with the radius.
    // Based on some experimental radius values and sigmas,
    // we approximately fit sigma = f(radius) as
    // sigma = radius * 0.4  + 0.6
    // The larger the radius gets, the more our gaussian blur
    // will resemble a box blur since with large sigma
    // the gaussian curve begins to lose its shape
    float sigma = 0.4f * radius + 0.6f;

    // Now compute the coefficients. We will store some redundant values to save
    // some math during the blur calculations precompute some values
    float coeff1 = 1.0f / (sqrtf(2.0f * pi) * sigma);
    float coeff2 = - 1.0f / (2.0f * sigma * sigma);

    float normalizeFactor = 0.0f;
    float floatR;
    int r;
    for (r = -radius; r <= radius; r ++) {
        floatR = (float)r;
        kernel[r + radius] = coeff1 * powf(e, floatR * floatR * coeff2);
        normalizeFactor += kernel[r + radius];
    }

    // Now we need to normalize the weights because all our coefficients need to add up to one
    normalizeFactor = 1.0f / normalizeFactor;
    for (r = -radius; r <= radius; r ++) {
        kernel[r + radius] *= normalizeFactor;
    }
}

void gaussianBlur(const uint8_t* in, uint8_t* out, size_t sizeX, size_t sizeY,
                  size_t vectorSize, int radius){
    float kernel[25 * 2 + 1];
    calculateGaussianWeights(kernel, radius);
    int x = 0;
    int xMin = 0;
    int xMax = sizeX * vectorSize - vectorSize;
    ALOGI("%i", xMax);
    int pixArraySize = (sizeX * sizeY) * vectorSize;
    int stride = sizeX * vectorSize;
    uint8_t tempOut[sizeX * sizeY * vectorSize];

    ALOGI("%i", pixArraySize);
    //horizontal blur pass
    for(int p = 0; p < pixArraySize; p++){
        float color = 0.0f;

        for(int r = -radius; r <= radius; r++){
            int pixInd = p + r * vectorSize;
            int safePixInd = std::min(std::max(pixInd, xMin), xMax);
            color += (float)out[safePixInd] * kernel[r + radius];
        }

        tempOut[p] = (uint8_t)color;

        x++;
        if(x == sizeX * vectorSize){
            xMin = xMax + vectorSize;
            xMax += sizeX * vectorSize;
            x = 0;
        }
    }

    int yMin = 0;
    int yMax = pixArraySize - stride;
    //vertical pass
    for(int p = 0; p < pixArraySize; p++){
        float color = 0;

        for(int r = -radius; r <= radius; r++){
            int pixInd = p + r * stride;
            int safePixInd = std::max(std::min(pixInd, yMax), yMin);

            color += (float) tempOut[safePixInd] * kernel[r + radius];
        }

        out[p] = (uint8_t)color;

        yMin++;
        yMax++;
        if(yMin == sizeX * vectorSize){
            yMin = 0;
            yMax = pixArraySize - stride;
        }
    }
}

extern "C" JNIEXPORT void JNICALL
Java_com_example_nativeblur_MainActivity_nativeBlur(
        JNIEnv *env,
        jobject,
        jobject input,
        jobject  output,
        jint radius) {
    BitmapGuard in{env, input};
    BitmapGuard out{env, output};
    gaussianBlur(in.get(), out.get(), in.width(), in.height(), in.vectorSize(), radius);
}

