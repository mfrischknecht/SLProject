//#############################################################################
//  File:      GLES3Camera2Service.java
//  Author:    Marcus Hudritsch
//  Date:      Spring 2017
//  Purpose:   Android camera2 service implementation
//  Codestyle: https://github.com/cpvrlab/SLProject/wiki/Coding-Style-Guidelines
//  Copyright: Marcus Hudritsch
//             This software is provide under the GNU General Public License
//             Please visit: http://opensource.org/licenses/GPL-3.0
//#############################################################################

// Please do not change the name space. The SLProject app is identified in the app-store with it.
package ch.fhnw.comgr;

import android.app.Service;
import android.content.Intent;
import android.graphics.ImageFormat;
import android.hardware.camera2.CameraAccessException;
import android.hardware.camera2.CameraCaptureSession;
import android.hardware.camera2.CameraCharacteristics;
import android.hardware.camera2.CameraDevice;
import android.hardware.camera2.CameraManager;
import android.hardware.camera2.CaptureRequest;
import android.hardware.camera2.params.StreamConfigurationMap;
import android.media.Image;
import android.media.ImageReader;
import android.os.IBinder;
import android.support.annotation.NonNull;
import android.util.Log;
import android.util.Size;

import java.util.Arrays;

/**
 * The camera service is started from the activity with cameraStart and
 * stopped with cameraStop. These methods are called from within the views
 * renderer whenever the displayed scene requests a video image.
 * See GLES3View.Renderer.onDrawFrame for the invocation.
 * Camera permission is checked in the activity at startup.
 */
@SuppressWarnings("MissingPermission")
public class GLES3Camera2Service extends Service {
    protected static final String TAG = "SLProject";
    public static int videoType = CameraCharacteristics.LENS_FACING_BACK;
    public static int requestedVideoSizeIndex = 0; // see getRequestedSize
    public static boolean isTransitioning = false;
    public static boolean isRunning = false;
    protected CameraDevice cameraDevice;
    protected CameraCaptureSession captureSession;
    protected ImageReader imageReader;

    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        Log.i(TAG, "GLES3Camera2Service.onStartCommand flags " + flags + " startId " + startId);

        CameraManager manager = (CameraManager) getSystemService(CAMERA_SERVICE);
        try {
            String pickedCamera = getCamera(manager, videoType);
            manager.openCamera(pickedCamera, cameraStateCallback, null);
            Size videoSize = getRequestedSize(manager, videoType, requestedVideoSizeIndex);
            imageReader = ImageReader.newInstance(videoSize.getWidth(), videoSize.getHeight(), ImageFormat.YUV_420_888, 2);
            imageReader.setOnImageAvailableListener(onImageAvailableListener, null);
            Log.i(TAG, "imageReader created");
        } catch (CameraAccessException e) {
            Log.e(TAG, e.getMessage());
        }

        return super.onStartCommand(intent, flags, startId);
    }

    /**
     * Returns the Camera Id which matches the field lensFacing
     * @param manager The manager got by getSystemService(CAMERA_SERVICE)
     * @param lensFacing LENS_FACING_BACK or LENS_FACING_FRONT
     */
    public String getCamera(CameraManager manager, int lensFacing) {
        try {
            for (String cameraId : manager.getCameraIdList()) {
                CameraCharacteristics characteristics = manager.getCameraCharacteristics(cameraId);
                int cOrientation = characteristics.get(CameraCharacteristics.LENS_FACING);
                if (cOrientation == lensFacing)
                    return cameraId;
            }
        } catch (CameraAccessException e) {
            e.printStackTrace();
        }
        return null;
    }


    /**
     * Returns the requested video size in pixel
     * @param manager The manager got by getSystemService(CAMERA_SERVICE)
     * @param lensFacing LENS_FACING_BACK or LENS_FACING_FRONT
     * @param requestedSizeIndex An index of 0 returns the default size of 640x480
     *                           If this size is not available the median size is returned.
     *                           An index of -1 return the next smaller one
     *                           An index of +1 return the next bigger one
     */
    private Size getRequestedSize(CameraManager manager,
                                  int lensFacing,
                                  int requestedSizeIndex) {

        Size[] availableSizes = getOutputSizes(manager, lensFacing);

        // set default size index to a size in the middle of the array
        int defaultSizeIndex = availableSizes.length / 2;

        // get the index of the 640x480 resolution
        for (int i=0; i< availableSizes.length; ++i) {
            int w = availableSizes[i].getWidth();
            int h = availableSizes[i].getHeight();
            if (w == 640 && h == 480) {
                defaultSizeIndex = i;
                break;
            }
        }

        if (defaultSizeIndex - requestedSizeIndex < 0)
            return availableSizes[0];
        else if (defaultSizeIndex - requestedSizeIndex >= availableSizes.length)
            return availableSizes[availableSizes.length-1];
        else
            return availableSizes[defaultSizeIndex - requestedSizeIndex];
    }

    /**
     * Returns an array of output sizes for the requested camera (front or back)
     * @param manager The manager got by getSystemService(CAMERA_SERVICE)
     * @param lensFacing LENS_FACING_BACK or LENS_FACING_FRONT
     */
    private Size[] getOutputSizes(CameraManager manager, int lensFacing) {
        try {
            for (String cameraId : manager.getCameraIdList()) {
                CameraCharacteristics characteristics = manager.getCameraCharacteristics(cameraId);
                int cOrientation = characteristics.get(CameraCharacteristics.LENS_FACING);
                if (cOrientation == lensFacing) {
                    StreamConfigurationMap streamConfigurationMap = characteristics.get(CameraCharacteristics.SCALER_STREAM_CONFIGURATION_MAP);
                    Size[] sizes = streamConfigurationMap.getOutputSizes(ImageReader.class);
                    return sizes;
                }
            }
        } catch (CameraAccessException e) {
            e.printStackTrace();
        }
        return null;
    }

    protected CameraDevice.StateCallback cameraStateCallback = new CameraDevice.StateCallback() {
        @Override
        public void onOpened(@NonNull CameraDevice camera) {
            Log.i(TAG, "CameraDevice.StateCallback onOpened");
            cameraDevice = camera;
            actOnReadyCameraDevice();
        }

        @Override
        public void onDisconnected(@NonNull CameraDevice camera) {
            Log.w(TAG, "CameraDevice.StateCallback onDisconnected");
        }

        @Override
        public void onError(@NonNull CameraDevice camera, int error) {
            Log.e(TAG, "CameraDevice.StateCallback onError " + error);
        }
    };

    protected CameraCaptureSession.StateCallback sessionStateCallback = new CameraCaptureSession.StateCallback() {
        @Override
        public void onConfigured(@NonNull CameraCaptureSession session) {
            Log.i(TAG, "CameraCaptureSession.StateCallback onConfigured");
            GLES3Camera2Service.this.captureSession = session;
            try {
                session.setRepeatingRequest(createCaptureRequest(), null, null);
                isTransitioning = false;
                isRunning = true;
            } catch (CameraAccessException e) {
                Log.e(TAG, e.getMessage());
            }
        }

        @Override
        public void onConfigureFailed(@NonNull CameraCaptureSession session) {
        }
    };

    protected ImageReader.OnImageAvailableListener onImageAvailableListener = new ImageReader.OnImageAvailableListener() {
        @Override
        public void onImageAvailable(ImageReader reader) {

            // The opengl renderer runs in its own thread. We have to copy the image in the renderers thread!
            GLES3Lib.view.queueEvent(new Runnable() {
                @Override
                public void run() {
                    //Log.i(TAG, "<" + Thread.currentThread().getId());
                    Image img = reader.acquireLatestImage();

                    if (img == null)
                        return;

                    // Check image format
                    int format = reader.getImageFormat();
                    if (format != ImageFormat.YUV_420_888) {
                        throw new IllegalArgumentException("Camera image must have format YUV_420_888.");
                    }

                    Image.Plane[] planes = img.getPlanes();

                    Image.Plane Y = planes[0];
                    Image.Plane U = planes[1];
                    Image.Plane V = planes[2];

                    int ySize = Y.getBuffer().remaining();
                    int uSize = U.getBuffer().remaining();
                    int vSize = V.getBuffer().remaining();

                    int yPixStride = Y.getPixelStride();
                    int uPixStride = Y.getPixelStride();
                    int vPixStride = Y.getPixelStride();

                    int yRowStride = Y.getRowStride();
                    int uRowStride = Y.getRowStride();
                    int vRowStride = Y.getRowStride();


                    byte[] data = new byte[ySize + uSize + vSize];
                    Y.getBuffer().get(data, 0, ySize);
                    U.getBuffer().get(data, ySize, uSize);
                    V.getBuffer().get(data, ySize + uSize, vSize);

                    ///////////////////////////////////////////////////////////////
                    GLES3Lib.copyVideoImage(img.getWidth(), img.getHeight(), data);
                    ///////////////////////////////////////////////////////////////

                    /*
                    byte[] bufY = new byte[ySize];
                    byte[] bufU = new byte[uSize];
                    byte[] bufV = new byte[vSize];

                    Y.getBuffer().get(bufY, 0, ySize);
                    U.getBuffer().get(bufU, 0, uSize);
                    V.getBuffer().get(bufV, 0, vSize);

                    // For future call of GLES3Lib.copyVideoYUVPlanes
                    GLES3Lib.copyVideoYUVPlanes(img.getWidth(), img.getHeight(),
                                                bufY, ySize, yPixStride, yRowStride,
                                                bufU, uSize, uPixStride, uRowStride,
                                                bufV, vSize, vPixStride, vRowStride);
                    */
                    img.close();
                }
            });
        }
    };


    public void actOnReadyCameraDevice() {
        try {
            cameraDevice.createCaptureSession(Arrays.asList(imageReader.getSurface()), sessionStateCallback, null);
        } catch (CameraAccessException e) {
            Log.e(TAG, e.getMessage());
        }
    }

    @Override
    public void onDestroy() {
        Log.i(TAG, "GLES3Camera2Service.onDestroy");
        try {
            captureSession.abortCaptures();
        } catch (CameraAccessException e) {
            Log.e(TAG, e.getMessage());
        }
        captureSession.close();
        cameraDevice.close();
        imageReader.close();

        isTransitioning = false;
        isRunning = false;
    }

    protected CaptureRequest createCaptureRequest() {
        try {
            CaptureRequest.Builder builder = cameraDevice.createCaptureRequest(CameraDevice.TEMPLATE_RECORD);
            builder.addTarget(imageReader.getSurface());
            return builder.build();
        } catch (CameraAccessException e) {
            Log.e(TAG, e.getMessage());
            return null;
        }
    }

    @Override
    public IBinder onBind(Intent intent) {
        return null;
    }
}