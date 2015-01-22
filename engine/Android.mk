LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

DERIVED_SRC = ../_cache/mac/Debug/kernel.build/DerivedSources

TARGET_PLATFORM=android-8

LOCAL_MODULE := revandroid-kernel

LOCAL_ARM_MODE := arm

LOCAL_CFLAGS := -DGL_GLEXT_PROTOTYPES=1

LOCAL_SRC_FILES := $(addprefix src/,\
	aclip.cpp block.cpp button.cpp buttondraw.cpp card.cpp cardlst.cpp \
	cdata.cpp chunk.cpp cmds.cpp cmdsc.cpp cmdse.cpp cmdsf.cpp \
	cmdsm.cpp cmdsp.cpp cmdss.cpp constant.cpp control.cpp cpalette.cpp \
	date.cpp debug.cpp dispatch.cpp dllst.cpp objectprops.cpp \
	express.cpp field.cpp fieldf.cpp fieldh.cpp fields.cpp fieldstyledtext.cpp fieldhtml.cpp fieldrtf.cpp \
	font.cpp funcs.cpp funcsm.cpp globals.cpp graphic.cpp \
	graphicscontext.cpp \
	group.cpp \
	handler.cpp hc.cpp hndlrlst.cpp ibmp.cpp idraw.cpp ifile.cpp igif.cpp iimport.cpp \
	ijpg.cpp \
	image.cpp \
	imagelist.cpp \
	imageloader.cpp \
	image_rep.cpp \
	image_rep_encoded.cpp \
	image_rep_mutable.cpp \
	image_rep_densitymapped.cpp \
	image_rep_resampled.cpp \
	imagebitmap.cpp ipng.cpp irle.cpp itransform.cpp iutil.cpp \
	keywords.cpp line.cpp literal.cpp magnify.cpp mcerror.cpp \
	mcio.cpp mcstring.cpp mctheme.cpp newobj.cpp mcutility.cpp newobj.cpp\
	object.cpp objectpropsets.cpp objptr.cpp operator.cpp paragraf.cpp paragrafattr.cpp param.cpp \
	property.cpp pickle.cpp \
	regex.cpp \
	region.cpp \
	resolution.cpp \
	scriptpt.cpp \
	scrolbar.cpp scrollbardraw.cpp segment.cpp sellst.cpp \
	stack.cpp \
	stack2.cpp \
	stack3.cpp \
	stacklst.cpp \
	stackview.cpp \
	statemnt.cpp styledtext.cpp tooltip.cpp \
	transfer.cpp uidc.cpp gradient.cpp edittool.cpp \
	undolst.cpp util.cpp variable.cpp vclip.cpp visual.cpp \
	eps.cpp mcssl.cpp opensslsocket.cpp socket_resolve.cpp notify.cpp fonttable.cpp \
	answer.cpp ask.cpp external.cpp player.cpp surface.cpp \
	combiners.cpp path.cpp metacontext.cpp \
	printer.cpp unicode.cpp rtf.cpp rtfsupport.cpp text.cpp \
	customprinter.cpp iquantization.cpp iquantize_new.cpp \
	pathgray.cpp pathprocess.cpp \
	objectstream.cpp \
	menuparse.cpp parentscript.cpp securemode.cpp \
	bitmapeffect.cpp bitmapeffectblur.cpp md5.cpp capsule.cpp \
	externalv0.cpp externalv1.cpp \
	mode_standalone.cpp lextable.cpp eventqueue.cpp sha1.cpp stacke.cpp \
	redraw.cpp tilecache.cpp tilecachesw.cpp tilecachegl.cpp \
	sysunxdate.cpp sysspec.cpp stackcache.cpp uuid.cpp \
	mblad.cpp mblcalendar.cpp mblcamera.cpp mblcontact.cpp \
	quicktime.cpp \
	mblcontrol.cpp mbldc.cpp mbldialog.cpp mblflst.cpp mblhandlers.cpp mblmain.cpp mblnotification.cpp \
	mblsensor.cpp mblspec.cpp mblsound.cpp mblstack.cpp mblstore.cpp mbltheme.cpp \
	mblandroid.cpp mblandroidbrowser.cpp  mblandroidbusyindicator.cpp \
	mblandroidcalendar.cpp mblandroidcamera.cpp mblandroidcontact.cpp mblandroidcontrol.cpp \
	mblandroiddc.cpp mblandroiddialog.cpp mblandroidfont.cpp mblandroidfs.cpp mblandroididletimer.cpp mblandroidinput.cpp \
	mblandroidio.cpp mblandroidjava.cpp mblandroidmail.cpp mblandroidmediapick.cpp mblandroidmisc.cpp mblandroidmm.cpp \
	mblandroidnetwork.cpp mblandroidnotification.cpp mblandroidorientation.cpp mblandroidplayer.cpp \
	mblandroidprocess.cpp mblandroidscroller.cpp mblandroidsensor.cpp mblandroidstore.cpp mblandroidsound.cpp \
	mblandroidtextlayout.cpp mblandroidtextmessaging.cpp mblandroidtypeface.cpp mblandroidurl.cpp mblnetwork.cpp\
	exec-ad.cpp exec-addressbook.cpp exec-busyindicator.cpp exec-calendar.cpp exec-idletimer.cpp exec-mail.cpp exec-misc.cpp \
	exec-nativecontrol.cpp exec-notification.cpp exec-orientation.cpp exec-pick.cpp \
	exec-sensor.cpp exec-sound.cpp exec-store.cpp exec-textmessaging.cpp \
	exec-array.cpp exec-datetime.cpp exec-engine.cpp exec-files.cpp exec-filters.cpp exec-interface.cpp \
	exec-logic.cpp exec-math.cpp exec-multimedia.cpp exec-network.cpp exec-pasteboard.cpp exec-scripting.cpp \
	exec-strings.cpp exec-strings-chunk.cpp exec-text.cpp exec-graphics.cpp exec-security.cpp exec-printing.cpp exec-debugging.cpp \
	exec-ide.cpp exec-server.cpp exec-interface2.cpp exec.cpp \
	exec-interface-aclip.cpp exec-interface-button.cpp exec-interface-card.cpp exec-interface-control.cpp \
	exec-interface-field.cpp exec-interface-field-chunk.cpp exec-interface-graphic.cpp exec-interface-group.cpp exec-interface-image.cpp \
	exec-interface-object.cpp exec-interface-player.cpp exec-interface-scrollbar.cpp exec-interface-stack.cpp \
	exec-interface-vclip.cpp exec-legacy.cpp exec-dialog.cpp exec-keywords.cpp \
	syntax.cpp \
	foundation-legacy.cpp legacy_spec.cpp \
	stacktile.cpp sysunxthreads.cpp \
	widget.cpp widget-events.cpp native-layer.cpp native-layer-android.cpp \
	module-canvas.cpp module-engine.cpp modules.cpp \
	exec-extension.cpp exec-keywords.cpp) \
	$(DERIVED_SRC)/canvas.mlc.c $(DERIVED_SRC)/engine.mlc.c $(DERIVED_SRC)/widget.mlc.c

LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/include \
	$(LOCAL_PATH)/../libfoundation/include \
	$(LOCAL_PATH)/../libgraphics/include \
	$(LOCAL_PATH)/../thirdparty/libpng/include \
	$(LOCAL_PATH)/../thirdparty/libpcre/include \
	$(LOCAL_PATH)/../thirdparty/libjpeg/include \
	$(LOCAL_PATH)/../thirdparty/libgif/include \
	$(LOCAL_PATH)/../libexternal/include \
	$(LOCAL_PATH)/../thirdparty/libskia/include/core \
	$(LOCAL_PATH)/../thirdparty/libskia/include/effects \
	$(LOCAL_PATH)/../thirdparty/libskia/include/config \
	$(LOCAL_PATH)/../thirdparty/libskia/include/ports \
	$(LOCAL_PATH)/../thirdparty/libfreetype/include \
	$(LOCAL_PATH)/../thirdparty/libopenssl/include \
	$(LOCAL_PATH)/../thirdparty/libffi/include \
	$(LOCAL_PATH)/../libscript/include

include $(BUILD_STATIC_LIBRARY)

#########

include $(CLEAR_VARS)

TARGET_PLATFORM=android-8

LOCAL_MODULE := revandroid-community

LOCAL_ARM_MODE := arm

LOCAL_CFLAGS := -DGL_GLEXT_PROTOTYPES=1

LOCAL_SRC_FILES := $(addprefix src/,stacksecurity.cpp mblandroidad.cpp)

LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/include \
	$(LOCAL_PATH)/../libfoundation/include \
	$(LOCAL_PATH)/../libgraphics/include

LOCAL_CFLAGS := -DGL_GLEXT_PROTOTYPES=1

LOCAL_GROUP_STATIC_LIBRARIES := true
LOCAL_STATIC_LIBRARIES := librevandroid-kernel libscript libffi libfoundation libgraphics libjpeg libpcre libpng libgif libskia libfreetype libharfbuzz libexpat_static openssl

LOCAL_LDLIBS += -lz -lm -llog -ljnigraphics -lGLESv1_CM

LOCAL_LDFLAGS := -Wl,--script=$(LOCAL_PATH)/standalone-android.link -L$(LOCAL_PATH)/../prebuilt/lib/android/armv6

include $(BUILD_SHARED_LIBRARY)
