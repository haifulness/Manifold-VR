LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

# Each app compiles GeometryGamesJNI.c into its own app-specific library,
# so there's no need to compile it here.  (Moreover, when I had originally
# tried compiling it here I couldn't get Eclipse/ADT to package the resulting
# library into each app's .apk file, which is why I gave up on that approach
# and decided to compile it into the app-specific libraries instead.
# The latter approach also avoids any possible issues with multiply defined
# functions and globals.)

LOCAL_MODULE		:= DummyModuleToAvoidNDKWarning

LOCAL_CFLAGS		:= -std=gnu99 -Wall -Wextra -DSUPPORT_OPENGL

LOCAL_C_INCLUDES	+= $(LOCAL_PATH)/../../../../Shared
LOCAL_C_INCLUDES	+= $(LOCAL_PATH)/../../../../Shared/GeometryGamesCore-Android
LOCAL_C_INCLUDES	+= $(LOCAL_PATH)/../../../../Shared/GeometryGamesUtilities
LOCAL_C_INCLUDES	+= $(LOCAL_PATH)/../../../../Shared/GL3

LOCAL_SRC_FILES		:= DummyFileToAvoidNDKWarning.c

include $(BUILD_SHARED_LIBRARY)
