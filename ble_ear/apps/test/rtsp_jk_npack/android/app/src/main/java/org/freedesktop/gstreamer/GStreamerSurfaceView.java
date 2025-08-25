//package org.freedesktop.gstreamer;
//
//import android.content.Context;
//import android.util.AttributeSet;
//import android.util.Log;
//import android.view.SurfaceView;
//
//// A simple SurfaceView whose width and height can be set from the outside
//public class GStreamerSurfaceView extends SurfaceView {
//    public int media_width = 320;
//    public int media_height = 240;
//
//    // Mandatory constructors, they do not do much
//    public GStreamerSurfaceView(Context context, AttributeSet attrs,
//                                int defStyle) {
//        super(context, attrs, defStyle);
//    }
//
//    public GStreamerSurfaceView(Context context, AttributeSet attrs) {
//        super(context, attrs);
//    }
//
//    public GStreamerSurfaceView(Context context) {
//        super(context);
//    }
//
//    // Called by the layout manager to find out our size and give us some rules.
//    // We will try to maximize our size, and preserve the media's aspect ratio if
//    // we are given the freedom to do so.
//    @Override
//    protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
//        int width = 0, height = 0;
//        int wmode = MeasureSpec.getMode(widthMeasureSpec);
//        int hmode = MeasureSpec.getMode(heightMeasureSpec);
//        int wsize = MeasureSpec.getSize(widthMeasureSpec);
//        int hsize = MeasureSpec.getSize(heightMeasureSpec);
//
//        Log.i ("GStreamer", "onMeasure called with " + media_width + "x" + media_height);
//        // Obey width rules
//        switch (wmode) {
//        case MeasureSpec.AT_MOST:
//            if (hmode == MeasureSpec.EXACTLY) {
//                width = Math.min(hsize * media_width / media_height, wsize);
//                break;
//            }
//        case MeasureSpec.EXACTLY:
//            width = wsize;
//            break;
//        case MeasureSpec.UNSPECIFIED:
//            width = media_width;
//        }
//
//        // Obey height rules
//        switch (hmode) {
//        case MeasureSpec.AT_MOST:
//            if (wmode == MeasureSpec.EXACTLY) {
//                height = Math.min(wsize * media_height / media_width, hsize);
//                break;
//            }
//        case MeasureSpec.EXACTLY:
//            height = hsize;
//            break;
//        case MeasureSpec.UNSPECIFIED:
//            height = media_height;
//        }
//
//        // Finally, calculate best size when both axis are free
//        if (hmode == MeasureSpec.AT_MOST && wmode == MeasureSpec.AT_MOST) {
//            int correct_height = width * media_height / media_width;
//            int correct_width = height * media_width / media_height;
//
//            if (correct_height < height)
//                height = correct_height;
//            else
//                width = correct_width;
//        }
//
//        // Obey minimum size
//        width = Math.max (getSuggestedMinimumWidth(), width);
//        height = Math.max (getSuggestedMinimumHeight(), height);
//        setMeasuredDimension(width, height);
//    }
//
//}


package org.freedesktop.gstreamer;

import android.content.Context;
import android.util.AttributeSet;
import android.util.Log;
import android.view.SurfaceView;

// A simple SurfaceView that can have its width and height adjusted,
// potentially for aspect ratio handling.
public class GStreamerSurfaceView extends SurfaceView {
    public int media_width = 320; // Default media width
    public int media_height = 240; // Default media height

    // Mandatory constructors.
    public GStreamerSurfaceView(Context context, AttributeSet attrs,
                                int defStyle) {
        super(context, attrs, defStyle);
    }

    public GStreamerSurfaceView(Context context, AttributeSet attrs) {
        super(context, attrs);
    }

    public GStreamerSurfaceView(Context context) {
        super(context);
    }

    // This method is called by the layout manager to determine the view's size.
    // It attempts to respect the parent's constraints and maintain the aspect ratio
    // of the media if possible.
    @Override
    protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
        int width = 0, height = 0;
        int wmode = MeasureSpec.getMode(widthMeasureSpec);
        int hmode = MeasureSpec.getMode(heightMeasureSpec);
        int wsize = MeasureSpec.getSize(widthMeasureSpec);
        int hsize = MeasureSpec.getSize(heightMeasureSpec);

        Log.i ("GStreamer", "onMeasure called with " + media_width + "x" + media_height);
        // Obey width rules from the parent.
        switch (wmode) {
            case MeasureSpec.AT_MOST:
                if (hmode == MeasureSpec.EXACTLY) {
                    // If height is fixed, calculate width based on aspect ratio.
                    width = Math.min(hsize * media_width / media_height, wsize);
                    break;
                }
            case MeasureSpec.EXACTLY:
                width = wsize; // Use the exact width provided by the parent.
                break;
            case MeasureSpec.UNSPECIFIED:
                width = media_width; // Use the media's intrinsic width if no constraints.
        }

        // Obey height rules from the parent.
        switch (hmode) {
            case MeasureSpec.AT_MOST:
                if (wmode == MeasureSpec.EXACTLY) {
                    // If width is fixed, calculate height based on aspect ratio.
                    height = Math.min(wsize * media_height / media_width, hsize);
                    break;
                }
            case MeasureSpec.EXACTLY:
                height = hsize; // Use the exact height provided by the parent.
                break;
            case MeasureSpec.UNSPECIFIED:
                height = media_height; // Use the media's intrinsic height if no constraints.
        }

        // If both width and height modes are AT_MOST, try to find the best fit
        // while maintaining the aspect ratio.
        if (hmode == MeasureSpec.AT_MOST && wmode == MeasureSpec.AT_MOST) {
            int correct_height = width * media_height / media_width;
            int correct_width = height * media_width / media_height;

            if (correct_height < height) {
                // If calculated height is less than available, use it.
                height = correct_height;
            } else {
                // Otherwise, use calculated width.
                width = correct_width;
            }
        }

        // Ensure the view is at least its minimum suggested size.
        width = Math.max (getSuggestedMinimumWidth(), width);
        height = Math.max (getSuggestedMinimumHeight(), height);
        setMeasuredDimension(width, height); // Apply the calculated dimensions.
    }
}