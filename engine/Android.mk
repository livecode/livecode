LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

TARGET_PLATFORM=android-8

LOCAL_MODULE := revandroid-kernel

LOCAL_ARM_MODE := arm

LOCAL_SRC_FILES := $(addprefix src/,\
	aclip.cpp block.cpp button.cpp buttondraw.cpp card.cpp cardlst.cpp \
	cdata.cpp chunk.cpp cmds.cpp cmdsc.cpp cmdse.cpp cmdsf.cpp \
	cmdsm.cpp cmdsp.cpp cmdss.cpp constant.cpp control.cpp cpalette.cpp \
	date.cpp debug.cpp dispatch.cpp dllst.cpp objectprops.cpp \
	exec-ad.cpp exec-addressbook.cpp exec-busyindicator.cpp exec-calendar.cpp exec-idletimer.cpp exec-mail.cpp exec-misc.cpp \
	exec-nativecontrol.cpp exec-notification.cpp exec-orientation.cpp exec-pick.cpp \
	exec-sensor.cpp exec-sound.cpp exec-store.cpp exec-textmessaging.cpp \
	execpt.cpp express.cpp field.cpp fieldf.cpp fieldh.cpp fields.cpp fieldstyledtext.cpp fieldhtml.cpp fieldrtf.cpp \
	font.cpp funcs.cpp funcsm.cpp globals.cpp graphic.cpp group.cpp \
	handler.cpp hc.cpp hndlrlst.cpp ibmp.cpp idraw.cpp ifile.cpp igif.cpp iimport.cpp \
	ijpg.cpp image.cpp image_rep.cpp image_rep_encoded.cpp image_rep_mutable.cpp \
	image_rep_transformed.cpp imagebitmap.cpp ipng.cpp irle.cpp itransform.cpp iutil.cpp \
	keywords.cpp line.cpp literal.cpp magnify.cpp mcerror.cpp \
	mcio.cpp mcstring.cpp mctheme.cpp newobj.cpp mcutility.cpp \
	object.cpp objectpropsets.cpp objptr.cpp operator.cpp paragraf.cpp paragrafattr.cpp param.cpp \
	property.cpp pxmaplst.cpp pickle.cpp regex.cpp \
	scriptpt.cpp scrolbar.cpp scrollbardraw.cpp sellst.cpp stack.cpp stack2.cpp \
	stack3.cpp stacklst.cpp statemnt.cpp styledtext.cpp tooltip.cpp \
	transfer.cpp uidc.cpp gradient.cpp edittool.cpp \
	undolst.cpp util.cpp variable.cpp vclip.cpp visual.cpp \
	eps.cpp mcssl.cpp notify.cpp fonttable.cpp \
	answer.cpp ask.cpp external.cpp player.cpp surface.cpp \
	combiners.cpp path.cpp contextscalewrapper.cpp metacontext.cpp \
	printer.cpp unicode.cpp rtf.cpp rtfsupport.cpp text.cpp \
	customprinter.cpp iquantization.cpp iquantize_new.cpp \
	objectstream.cpp \
	menuparse.cpp parentscript.cpp securemode.cpp \
	bitmapeffect.cpp bitmapeffectblur.cpp md5.cpp capsule.cpp \
	externalv0.cpp externalv1.cpp \
	mode_standalone.cpp lextable.cpp eventqueue.cpp sha1.cpp stacke.cpp \
	redraw.cpp tilecache.cpp tilecachesw.cpp tilecachegl.cpp sysregion.cpp \
	sysunxdate.cpp sysspec.cpp stackcache.cpp \
	mblad.cpp mblalert.cpp mblbusyindicator.cpp mblcalendar.cpp mblcamera.cpp mblcontact.cpp \
	mblcontrol.cpp mbldc.cpp mbldialog.cpp mblflst.cpp mblhandlers.cpp mblmain.cpp mblnotification.cpp \
	mblsensor.cpp mblspec.cpp mblsound.cpp mblstack.cpp mblstore.cpp mbltextmessaging.cpp \
	mblandroid.cpp mblandroidad.cpp mblandroidalert.cpp mblandroidbrowser.cpp  mblandroidbusyindicator.cpp \
	mblandroidcalendar.cpp mbandroidcamera.cpp mblandroidcontact.cpp mblandroidcontext.cpp mblandroidcontrol.cpp \
	mblandroiddc.cpp mblandroiddialog.cpp mblandroidfont.cpp mblandroidfs.cpp mblandroididletimer.cpp mblandroidinput.cpp \
	mblandroidio.cpp mblandroidjava.cpp mblandroidmail.cpp mblandroidmisc.cpp mblandroidmm.cpp \
	mblandroidnetwork.cpp mblandroidnotification.cpp mblandroidorientation.cpp mblandroidplayer.cpp \
	mblandroidprocess.cpp mblandroidscroller.cpp mblandroidsensor.cpp mblandroidstore.cpp mblandroidsound.cpp \
	mblandroidtextlayout.cpp mblandroidtextmessaging.cpp mblandroidurl.cpp )

LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/include \
	$(LOCAL_PATH)/../libfoundation/include \
	$(LOCAL_PATH)/../thirdparty/libpng/include \
	$(LOCAL_PATH)/../thirdparty/libpcre/include \
	$(LOCAL_PATH)/../thirdparty/libjpeg/include \
	$(LOCAL_PATH)/../thirdparty/libgif/include \
	$(LOCAL_PATH)/../libexternal/include \
	$(LOCAL_PATH)/../thirdparty/libskia/include/core \
	$(LOCAL_PATH)/../thirdparty/libskia/include/effects \
	$(LOCAL_PATH)/../thirdparty/libskia/include/config \
	$(LOCAL_PATH)/../thirdparty/libskia/include/ports \
	$(LOCAL_PATH)/../thirdparty/libfreetype/include

include $(BUILD_STATIC_LIBRARY)

#########

include $(CLEAR_VARS)

TARGET_PLATFORM=android-8

LOCAL_MODULE := revandroid-community

LOCAL_ARM_MODE := arm

LOCAL_SRC_FILES := $(addprefix src/,stacksecurity.cpp mblandroidad.cpp)

LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/include \
	$(LOCAL_PATH)/../libfoundation/include

LOCAL_STATIC_LIBRARIES := librevandroid-kernel libfoundation libjpeg libpcre libpng libgif libskia libfreetype

LOCAL_LDLIBS += -lz -lm -llog -ljnigraphics -lGLESv1_CM

ifneq ($(MODE),debug)
LOCAL_LDFLAGS := -Wl,--script=$(LOCAL_PATH)/standalone-android.link
endif

include $(BUILD_SHARED_LIBRARY)
