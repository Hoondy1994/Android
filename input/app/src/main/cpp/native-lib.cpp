#include <jni.h>
#include <android/input.h>

namespace {

const char* MotionActionName(int action) {
    switch (action) {
        case AMOTION_EVENT_ACTION_DOWN: return "DOWN";
        case AMOTION_EVENT_ACTION_UP: return "UP";
        case AMOTION_EVENT_ACTION_MOVE: return "MOVE";
        case AMOTION_EVENT_ACTION_CANCEL: return "CANCEL";
        case AMOTION_EVENT_ACTION_OUTSIDE: return "OUTSIDE";
        case AMOTION_EVENT_ACTION_POINTER_DOWN: return "POINTER_DOWN";
        case AMOTION_EVENT_ACTION_POINTER_UP: return "POINTER_UP";
        case AMOTION_EVENT_ACTION_HOVER_MOVE: return "HOVER_MOVE";
        case AMOTION_EVENT_ACTION_SCROLL: return "SCROLL";
        case AMOTION_EVENT_ACTION_HOVER_ENTER: return "HOVER_ENTER";
        case AMOTION_EVENT_ACTION_HOVER_EXIT: return "HOVER_EXIT";
        case AMOTION_EVENT_ACTION_BUTTON_PRESS: return "BUTTON_PRESS";
        case AMOTION_EVENT_ACTION_BUTTON_RELEASE: return "BUTTON_RELEASE";
        default: return "UNKNOWN";
    }
}

const char* ToolTypeName(int tool) {
    switch (tool) {
        case AMOTION_EVENT_TOOL_TYPE_FINGER: return "FINGER";
        case AMOTION_EVENT_TOOL_TYPE_STYLUS: return "STYLUS";
        case AMOTION_EVENT_TOOL_TYPE_MOUSE: return "MOUSE";
        case AMOTION_EVENT_TOOL_TYPE_ERASER: return "ERASER";
        case AMOTION_EVENT_TOOL_TYPE_PALM: return "PALM";
        case AMOTION_EVENT_TOOL_TYPE_UNKNOWN: return "UNKNOWN";
        default: return "OTHER";
    }
}

const char* AxisName(int axis) {
    switch (axis) {
        case AMOTION_EVENT_AXIS_X: return "AXIS_X";
        case AMOTION_EVENT_AXIS_Y: return "AXIS_Y";
        case AMOTION_EVENT_AXIS_PRESSURE: return "AXIS_PRESSURE";
        case AMOTION_EVENT_AXIS_SIZE: return "AXIS_SIZE";
        case AMOTION_EVENT_AXIS_TOUCH_MAJOR: return "AXIS_TOUCH_MAJOR";
        case AMOTION_EVENT_AXIS_TOUCH_MINOR: return "AXIS_TOUCH_MINOR";
        case AMOTION_EVENT_AXIS_ORIENTATION: return "AXIS_ORIENTATION";
        case AMOTION_EVENT_AXIS_VSCROLL: return "AXIS_VSCROLL";
        case AMOTION_EVENT_AXIS_HSCROLL: return "AXIS_HSCROLL";
        default: return "AXIS_OTHER";
    }
}

jstring ToJString(JNIEnv* env, const char* text) {
    return env->NewStringUTF(text);
}

}  // namespace

extern "C" JNIEXPORT jstring JNICALL
Java_com_example_inputtest_input_InputNativeBridge_actionName(
        JNIEnv* env, jobject /* this */, jint action) {
    const int masked = action & AMOTION_EVENT_ACTION_MASK;
    return ToJString(env, MotionActionName(masked));
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_example_inputtest_input_InputNativeBridge_toolTypeName(
        JNIEnv* env, jobject /* this */, jint toolType) {
    return ToJString(env, ToolTypeName(toolType));
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_example_inputtest_input_InputNativeBridge_axisName(
        JNIEnv* env, jobject /* this */, jint axis) {
    return ToJString(env, AxisName(axis));
}
