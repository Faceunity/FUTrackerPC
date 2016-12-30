#pragma once
#ifndef DDE_FACE_H
#define DDE_FACE_H
#include <stdlib.h>

/// \brief the minimum value of output expression coefficients
#define EXPR_COEF_MIN -0.2f
/// \brief the maximum value of output expression coefficients
#define EXPR_COEF_MAX 1.5f

/// \brief the number of identity coefficients by the system
#define N_IDENTITIES 32
/// \brief the number of expressions supported by the system
#define N_EXPRESSIONS 47
/// \brief the number of 3D landmarks supported by the system
#define N_3D_LANDMARKS 75

// standard C/C++ header boilerplate
#if defined(_WIN32)&&!defined(USE_STATIC_LIB_WIN)
#define EXPORT __declspec(dllimport)
#else
#define EXPORT
#endif

#ifdef __cplusplus
extern "C"{
#endif

/***************************************************************
Here goes the high-level "easydde" API. It should be enough for
simple applications that only tracks one face.
***************************************************************/

/**
\brief Initialize the library and authenticate it to our
       licensing server. This function must be called once and
       *ONLY* once before all other functions.
\param p points to the content of v3.bin. That file should have
       been provided alongside this header. You can free the
       memory after `dde_setup` returns.
\param authdata points to the authentication package.
       If you haven't purchased a license yet, pass NULL here
       to start your evaluation. An evaluation version stops
       tracking after a few minutes' worth of frames.
\param authdata_sz is the size of the authentication package.
       It's declared as an int to avoid stdint dependency.
*/
EXPORT int dde_setup(const void* p, const void* authdata,int authdata_sz);

/// \brief mask for the image format bits
#define FLAG_IMAGE_FORMAT_MASK 3
/// \brief the standard RGBA format with 8 bits per channel
#define FLAG_IMAGE_FORMAT_RGBA 0
/// \brief a greyscale image with 8 bits per pixel
#define FLAG_IMAGE_FORMAT_GRAYSCALE 1
/// \brief the standard BGRA format with 8 bits per channel
#define FLAG_IMAGE_FORMAT_BGRA FLAG_IMAGE_FORMAT_RGBA
/**
\brief NV21, the Android YUV format, where the Y channel is
       interpreted as a greyscale image
*/
#define FLAG_IMAGE_FORMAT_NV21 FLAG_IMAGE_FORMAT_GRAYSCALE
/**
\brief NV12, the iOS YUV format, where the Y channel is
       interpreted as a greyscale image
*/
#define FLAG_IMAGE_FORMAT_NV12 FLAG_IMAGE_FORMAT_GRAYSCALE
/**
\brief whatever YUV420 format, where the Y channel is 
       interpreted as a greyscale image
*/
#define FLAG_IMAGE_FORMAT_I420 FLAG_IMAGE_FORMAT_GRAYSCALE
///////////
/**
\brief disables image orientation detection, which makes the
       tracking faster but you'll need to set the correct
       orientation manually using `easydde_set_rotation_mode`.
*/
#define FLAG_DISABLE_ROTATION 4
/**
\brief disable `ddear_`-related features, which makes the
       tracking faster
*/
#define FLAG_DISABLE_AR 8
/**
\brief disables side-face detection, which makes the
       tracking faster
*/
#define FLAG_DISABLE_SIDE_FACE 16

/**
\brief Feed an image frame to the tracker
\param img points to the image data
\param stride specifies the distance between a pixel and 
       the pixel on the next row, in bytes.
       - On iOS, the stride can be obtained by calling 
       CVPixelBufferGetBytesPerRowOfPlane.
       - On Android, the stride is just w if you use NV21.
\param w is the image width, in pixels
\param h is the image height, in pixels
\param flags packs the image format and miscellaneous options
\return 1 when a face is successfully tracked and useful 
          information can be obtained by calling 
          `easydde_get_data`
        0 when a face is being tracked, but the system is
          unconfident about the result returned by
          `easydde_get_data`.
          In this case, it's usually better to stick with
          whatever data you've obtained in the previous
          frame.
       -1 when the tracker is unable to detect a face
*/
EXPORT int easydde_run_ex(const void* img,int stride,int w,int h,int flags);
/**
\brief An obsolete version of `easydde_run_ex` without stride support.
       This interface is only provide for compatibility.
       Please use `easydde_run_ex` instead.
*/
EXPORT int easydde_run(const void* img,int w,int h,int flags);

/**
\brief Get the size of parameter `s`, in number of `float` numbers.
\param s is the name of a parameter. Refer to `easydde_get_data`
       for details.
*/
EXPORT int easydde_get_size(char* s);
/**
\brief Get the values of face parameter `s`

Example:
	int n=easydde_get_size("expression");
	float* expression=calloc(n, sizeof(float));
	easydde_get_data(expression, n, s);

\param s Choices for `s` include:
    "rotation"
        returns a quaternion representing the head rotation
    "translation"
        returns a 3D vector representing the head translation
    "expression"
        returns a vector with N_EXPRESSIONS-1 components representing
        the user expression. Each component of the vector is a blendshape 
        coefficient.
        Refer to:
            https://beta.modelo.io/projects/57d223f4df427650465ad6cb
        for a list of key blendshapes (described in Mandarin).
    "identity"
    	returns a vector with N_IDENTITIES components representing
    	the user identity. It's mainly useful with `ddear_query_database`
    "landmarks"
        returns N_3D_LANDMARKS*2 floats, which are the image-space
        positions of the facial landmark points.
    "landmarks_ar"
        returns N_3D_LANDMARKS*3 floats, which are the 3D spacial
        positions of the facial landmark points. The positions are
        rotated and translated.
    "face_confirmation_failure_stress"
        returns a floating point number indicating the accumulated
        "stress" returned by our internal failure detector. It can
        be interpreted as the number of "bad frames" that have 
        occured up to this point.
*/
EXPORT int easydde_get_data(float* ret,int szret, const char* s);
/// \brief Returns whether a face is being tracked
EXPORT int easydde_hasface();
/**
\brief Discard all current results and restart the tracker 
       *from scratch*. Usually, this function should only be
       called before you resume the tracker after a long pause
       or before you switch to another camera.
       `easydde_reset` only resets the tracker context backing 
       `easydde' functions.
*/
EXPORT void easydde_reset();

/**
\brief Get the detected face orientation in the provided image.
	(e.g. landscape, landscape reversed, portrait, portrait reversed).
\returns A number that is 0, 1, 2, or 3.
*/
EXPORT int easydde_get_rotation_mode();
/**
\brief Set the default face orientation in the face detector.
       Setting a correct orientation can reduce the starting
       latency considerably.
\param mode is a number that is 0, 1, 2, or 3.
       To stay on the safe side, avoid interpreting the orientation
       numbers. Just stick a face in front of your phone, print
       the return value of `easydde_get_rotation_mode()`, and
       hard-code the value for `easydde_set_default_orientation()`.
*/
EXPORT void easydde_set_default_orientation(int rmode);

/** \brief Obsolete API provided for compatibility*/
EXPORT void easydde_set_rotation_mode(int mode);
/** \brief Obsolete API provided for compatibility*/
EXPORT int easydde_get_default_orientation();

/**
\brief Get the default number of times the tracker is run for each frame
\return the current number of tracker runs per frame
*/
EXPORT int easydde_get_default_n_copies();
/**
\brief Set the default number of times the tracker is run for each frame
\param n_copies is the new number of tracker runs per frame
*/
EXPORT void easydde_set_default_n_copies(int n_copies);

/***************************************************************
Here goes the low-level API. These functions expose much more
control over the tracking process, which allows one to track
multiple faces at the same time. However, using them requires
understanding the inner workings of face detectors and trackers.
***************************************************************/

/**
\brief Get a global instance of our face detector.
\return A face detector handle to be passed to the other detector
        functions.
*/
EXPORT void* dde_facedet_get_global_instance();
/**
\brief Set a face detector parameter. 
\param name is the parameter name, it can be:
	"scaling_factor"
	"step_size"
	"size_min"
	"size_max"
	"min_neighbors"
	"min_required_variance"
	"is_mono"
\param value points to the new parameter value
\remark 
We use a sliding window detector, which has to make a lot of
tradeoffs between detection rate and performance. Most parameters 
must be tweaked / randomized on a per-frame basis to achieve an
overall good performance. Here is what we do inside the `easydde`
functions:

static int g_ez_rmode=0;
static int g_ez_state=0;

void tweak_detector_parameters(int w, int h){
	void* fd_ctx=dde_facedet_get_global_instance();
	// Randomize the minimal face size to cover the default scaling_factor of 1.2
	float size_min=((50.f/480.f)+(float)rand()*(1.f/(float)RAND_MAX)*(20.f/480.f))*(float)h;
	float min_neighbors=3.f;
	int rmode=0;
	int detector_type=0;
	// Try a different orientation each frame
	rmode=g_ez_rmode;
	g_ez_rmode=(int)(4.f*(float)rand()/(float)RAND_MAX);
	g_ez_rmode&=3;
	// Try the side-face detector occasionally
	if(!(flags&FLAG_DISABLE_SIDE_FACE)){
		if(rand()<(RAND_MAX>>1)){
			// The side-face detector has less false positives and min_neighbors==1 is enough
			detector_type=1+(rand()<(RAND_MAX>>1));
			min_neighbors=1.f;
		}
	}
	dde_facedet_set(fd_ctx,"size_min",&size_min);
	dde_facedet_set(fd_ctx,"min_neighbors",&min_neighbors);
}
*/
EXPORT int dde_facedet_set(void* detector,const char* name,const float* pvalue);
#define DETECTOR_TYPE_FRONTAL_FACE 0
#define DETECTOR_TYPE_RIGHT_SIDE_FACE 1
#define DETECTOR_TYPE_LEFT_SIDE_FACE 2
/**
\brief Run the face detector on an image.
\param detector is the detector handle
\param img points to the image data
\param stride specifies the distance between a pixel and
       the pixel on the next row, in bytes. Refer to
       `easydde_run_ex` for details.
\param w0 is the image width, in pixels
\param h0 is the image height, in pixels
\param ret receives a list of detected face rectangles:
	for each triangle i
		ret[i*4+0] is the x coordinate
		ret[i*4+1] is the y coordinate
		ret[i*4+2] is the width
		ret[i*4+3] is the height
\param max_faces indicates the maximum number of faces to detect.
\param rotation_mode is the detector orientation, which can be
       0, 1, 2, or 3. To stay on the safe side, one can avoid 
       interpreting the numbers and just try all of them.
\param detector_type can be:
	DETECTOR_TYPE_FRONTAL_FACE
	DETECTOR_TYPE_RIGHT_SIDE_FACE
	DETECTOR_TYPE_LEFT_SIDE_FACE
\return the number of faces detected
\remark
    Our detector is mainly designed for video applications and 
is designed to be very fast at the cost of a low detection rate
for any given frame, which is around 50%. If one uses randomized 
rotation / detector settings, it could take 10+ frames before a 
face gets detected. A fixed `rotation_mode` can reduce this 
number to around 2.
*/
EXPORT int dde_facedet_run_ex2(
	void* detector,
	const void* img,int stride,int w0,int h0, int* ret,int max_faces,
	int rotation_mode,int detector_type);

/** \brief Obsolete API provided for compatibility*/
EXPORT void* dde_facedet_create();
/** \brief Obsolete API provided for compatibility*/
EXPORT void dde_facedet_destroy(void* detector);
/** \brief Obsolete API provided for compatibility*/
EXPORT int dde_facedet_run(
	void* detector,
	void* img,int w,int h,
	int* ret,int max_faces,
	int rotation_mode);
/** \brief Obsolete API provided for compatibility*/
EXPORT int dde_facedet_run_ex(
	void* detector,
	void* img,int w0,int h0, int* ret,int max_faces,
	int rotation_mode,int detector_type);

/*
On x86 / x64 platforms, the v3.bin content and the TWorkArea type
*must* be aligned to 16-byte boundaries. The ALIGNED_TYPE is
declared to remind compilers of that.
*/
#if defined(_M_X86)||defined(_M_IX86)||defined(__i386__)
#include <emmintrin.h>
typedef __m128 ALIGNED_TYPE;
#else
typedef double ALIGNED_TYPE;
#endif

/**
\brief A placeholder type of a face-tracking context, it does 
not match the actual implementation, so don't use it to declare
variables. To create a context, use `dde_create_context()`.

Each context tracks one face. To track `n` faces, create `n`
independent instances of `TWorkArea`.
*/
typedef struct{
	ALIGNED_TYPE dummy;
}TWorkArea;

/// \brief Get the internal context used by `easydde` functions
EXPORT TWorkArea* easydde_get_context();

/// \brief Get the real size of `TWorkArea`
EXPORT size_t dde_context_size();
/// \brief Create an instance of `TWorkArea`
#define dde_create_context() calloc(1, dde_context_size())
/// \brief Destroy an instance of `TWorkArea`
#define dde_destroy_context(context) free(context)

/**
\brief Initialize a face tracker context from a detected face 
       rectangle. This function only sets up a context for the 
       tracker to run without performing any actual tracking.
       You can tweak additional parameters in-between.
\param context is the tracker context
\param rect is the face rectangle, which doesn't
	rect[0] is the x coordinate of the top-left corner
	rect[1] is the y coordinate of the top-left corner
	rect[2] is the x coordinate of the bottom-right corner
	rect[3] is the y coordinate of the bottom-right corner
\param w is the image width
\param h is the image height
\param modes is `rotation_mode+4*detector_type`. Refer to
       `dde_facedet_run_ex2` for `rotation_mode` and 
       `detector_type`.
\param pfl points to the camera focal length in pixels. You can
       set it to NULL to get a default value optimized for phones.
*/
EXPORT void dde_init_context_ex(TWorkArea* context,const float* rect,int w,int h,int modes,const float* pfl);

/**
\brief Feed an image frame to a tracker context
\param img points to the image data
\param stride specifies the distance between a pixel and 
       the pixel on the next row, in bytes. Refer to
       `easydde_run_ex` for details.
\param w is the image width, in pixels
\param h is the image height, in pixels
*/
EXPORT int hldde_next(TWorkArea* context, void* img,int stride,int w,int h);
#define dde_track(context,img,stride,w,h) hldde_next(context,img,stride,w,h)

/**
\brief Get a face parameter. It's mostly similar to `easydde_get_data`.
       A key difference is that `dde_get` returns a pointer that points
       to somewhere inside `context`.
\param context is the tracker context
\param name is the parameter name
\param pdim receives the number of floats available at the returned
       pointer
\return A pointer that points to the face parameter values
*/
EXPORT float* dde_get(TWorkArea* context,const char* name,int* pdim);

/** \brief Obsolete API provided for compatibility*/
EXPORT int hldde_first(TWorkArea* context, const void* img,int stride,int w,int h, const float* bb);
/** \brief Obsolete API provided for compatibility*/
EXPORT int hldde_first_ex(TWorkArea* context, const void* img,int stride,int w,int h, const float* bb,int rotation_mode,const float* pfl);

/** 
\brief Internal API for tweaking tracker parameters, consult 
       support for a list of available tweaks.
*/
EXPORT int dde_set(TWorkArea* context, const char* name,void* pval);
/** 
\brief Internal API for debugging, consult support when you
       need debugging.
*/
EXPORT void dde_debug_get_normalized_face_image(TWorkArea* context, const void* img,int stride,int w,int h, int* ret);
/**
\brief Internal API for computing face silhouette
\param context is the tracker context
\param cont_pids receives the dense output
    The dense output packs each silhouette vertex in two ints and two floats
    The ints are the vertex ids. The floats are the interpolation weights.
\param size_cont_pids is the number of ints available at cont_pids
       If there weren't enough space, no data would be written to cont_pids.
\param vkv2 is the output buffer for vertex ids and interpolation weights. 
	To access the weights, cast the POINTER type, i.e., ((float*)vkv2)[i*8+4]. 
	We need 8*N_SIL_POINTS elements of space.
	If you don't need vkv2, just set it to NULL.
\param identitiy_coefs_override can override the current user identity in the context.
	If you don't need it, set it to NULL.
\param dde_coefs_override can override the current face coefficients in the context.
	If you don't need it, set it to NULL.
\return the number of ints required for cont_pids
*/
EXPORT int dde_compute_silhouette(TWorkArea* context, int* cont_pids,int size_cont_pids,int* vkv2, const float* identitiy_coefs_override,const float* dde_coefs_override);

/***************************************************************
Here goes the AR (Augmented Reality) API. These functions can
compute a 3D rough model that approximates the user face. One can
use this model to overlay textures / ... on top of the camera
image. Calling these functions incurs an additional cost, though.

Consult the documentation for more details.
***************************************************************/

/**
\brief Get the static portion of the AR model data
\param ppuv receives a `short*`, which will point to the 2D texture 
       coordinates packed as `n_vertices*2` unsigned shorts.
\param ppebo receives a `short*`, which will point to an index 
       buffer with `n_triangles*3` shorts. It's intended to be
       rendered as `GL_TRIANGLES`.
\param pn_vertices receives `n_vertices`, the number of vertices
\param pn_triangles receives `n_triangles`, the number of triangles
*/
EXPORT int ddear_get_static_data_v3(short** ppuv,short** ppebo,int* pn_vertices,int* pn_triangles);

/**
\brief Refine the face tracker results for AR applications.
       This function has a noticeable cost, so `easydde_run_ex`
       does not call it by default.
\param context is the tracker context
\param img points to the image data
\param stride specifies the distance between a pixel and 
       the pixel on the next row, in bytes. Refer to
       `easydde_run_ex` for details.
\param w is the image width, in pixels
\param h is the image height, in pixels
\param search_range is a reserved parameter. Just set it to 0.
*/
EXPORT void ddear_run_optical_flow(TWorkArea* context, const void* img,int stride,int w,int h,int search_range);

/**
\brief Get the AR model vertex positions and a view matrix that 
       matches the current face in a given tracker context
\param context is the tracker context
\param ppv receives a `float*` with `n_vertices*3` floats, which
       are the vertex positions
\param pmatrix receives 16 floats, which are the view matrix
\remark
    `ddear_get_vertices` and `dde_get` work on the *same* buffer 
    and can overwrite each other's results
*/
EXPORT int ddear_get_vertices(TWorkArea* context, float** ppv,float* pmatrix);
/**
\brief Get the projection matrix required to render an AR model
\param context is the tracker context
\param pz_near points to a positive number that specifies the near
       clipping plane. Use NULL for a default value.
\param pz_far points to a positive number that specifies the far
       clipping plane. Use NULL for a default value.
\param pprojection receives 16 floats, which are the projection 
       matrix
\remark Our coordinate system uses a distance unit that roughly 
        corresonds to a screen-space pixel. The distance between
        the face and the camera is usually on the order of 10^3.
*/
EXPORT int ddear_get_projection_matrix(TWorkArea* context, const float* pz_near,const float* pz_far, float* pprojection);
/**
\brief Query the internal mesh database with a given set of identity
       and expression coefficients to generate a corresponding AR
       model.
\param ret receives `n_vertices*3` floats, which are the vertex 
       positions in the generated model
\param identitiy_coefs are the N_IDENTITIES identity coefficients
\param expr_coefs are the N_EXPRESSIONS-1 expression coefficients
*/
EXPORT void ddear_query_database(float* ret, const float* identitiy_coefs,const float* expr_coefs);

/**
\brief Compute per-vertex normal from the vertex data returned by 
       `ddear_get_vertices` or `ddear_query_database`
\param v_N receives the output normals, the vectors will NOT be 
       normalized. We expect them to be normalized later in a shader.
\param ar_vertices points to the vertex positions as returned by 
       `ddear_get_vertices` or `ddear_query_database`
*/
EXPORT void ddear_compute_normal(float* v_N,const float* ar_vertices);

/**
\brief Discard all current results and restart the tracker
       *from scratch*. Usually, this function should only be
       called before you resume the tracker after a long pause
       or before you switch to another camera.
       `easymultiface_reset` only resets the tracker context backing
       `easymultiface' functions.
*/
EXPORT void easymultiface_reset();

/**
\brief Feed an image frame to the multi-face tracker
\param p_invalidation_mask receives a bitmask of invalidated
       faces in this frame. The i-th bit being set in the
       returned mask indicates that the tracking result has
       been lost for the i-th face.
\param img points to the image data
\param stride specifies the distance between a pixel and 
       the pixel on the next row, in bytes.
       - On iOS, the stride can be obtained by calling 
       CVPixelBufferGetBytesPerRowOfPlane.
       - On Android, the stride is just w if you use NV21.
\param w is the image width, in pixels
\param h is the image height, in pixels
\param flags is the image format
\return
  A bitmask of tracked faces in this frame. The i-th
  bit being set in the returned mask indicates that
  you can obtain valid, updated parameters from
  `easymultiface_get_context(i)`.
  
  The face ids are consistent across frames. Here is an 
  example interaction sequence:
  - Person A enters the camera
    `easymultiface_run` detects A's face and returns 0x01,
    indicating that face 0 has valid results.
  - Person B enters the camera
    `easymultiface_run` detects B's face and returns 0x03,
    indicating that both face 0 and 1 have valid results.
  - Person A leaves the camera
    `easymultiface_run` fails to track A's face and
    returns 0x02, indicating that face 1, i.e., B's face,
    still has valid results.
    
    It would also return 0x01 in `*p_invalidation_mask`,
    indicating that face 0, i.e., A's face, has been 
    invalidated.
*/
EXPORT int easymultiface_run(int* p_invalidation_mask, const void* img, int stride_dde,int w, int h,int flags);

/**
\brief Get the internal context for a specific face tracked by 
       `easymultiface_run`
\param face_id is the face id
\return The context for that face
*/
EXPORT TWorkArea* easymultiface_get_context(int face_id);

/**
\brief Set the maximum number of faces we track. The default value is 4.
\param n is the new maximum number of faces to track
\return The previous maximum number of faces we track
*/
EXPORT int easymultiface_set_max_faces(int n_max_faces);

/**
\brief Get the maximum number of faces we track.
\return The maximum number of faces we track
*/
EXPORT int easymultiface_get_max_faces();

#ifdef __cplusplus
}
#endif
#undef EXPORT

#endif
