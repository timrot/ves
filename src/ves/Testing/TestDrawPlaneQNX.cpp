#include <stdio.h>

#include <gf/gf.h>
#include <gf/gf3d.h>
#include <GLES/gl.h>
#include <GLES/egl.h>
//#include </usr/include/GLES/glues.h>

gf_dev_t    gfdev;
gf_layer_t  layer;
int         layer_idx;

static EGLDisplay display;
static EGLSurface surface;

//////////////////////////////////////////////////////////////////////////////////////////

static EGLint attribute_list[]=
{
  EGL_NATIVE_VISUAL_ID, 0,
  EGL_NATIVE_RENDERABLE, EGL_TRUE,
  EGL_RED_SIZE, 5,
  EGL_GREEN_SIZE, 5,
  EGL_BLUE_SIZE, 5,
  EGL_DEPTH_SIZE, 16,
  EGL_NONE
};


//////////////////////////////////////////////////////////////////////////////////////////

void init_scene(int width, int height)
{
  /* Clear error */
  glGetError();

  /* Setup display viewport */
  glViewport(0, 0, (GLint)width, (GLint)height);

  /* draw a perspective scene */
  glMatrixMode(GL_PROJECTION);
  glFrustumf(-100.f, 100.f, -100.f, 100.f, 320.f, 640.f);
  glMatrixMode(GL_MODELVIEW);

  /* turn on features */
  glDisable(GL_DEPTH_TEST);


  if (glGetError())
  {
    printf("Oops! I screwed up my OpenGL ES calls somewhere\n");
  }
}
//////////////////////////////////////////////////////////////////////////////////////////

void render_scene()
{
  glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
  glClearColor(1.0, 1.0, 1.0, 1.0);

  fprintf(stderr, "Render\n");

  const GLfloat line[] = {
    0, 0, //point A
    50, 50, //point B
  };

  glVertexPointer(2, GL_FLOAT, 0, line);
  glEnableClientState(GL_VERTEX_ARRAY);
  glDrawArrays(GL_LINES, 0, 2);

  glDisableClientState(GL_VERTEX_ARRAY);

  if (glGetError())
  {
    printf("Oops! I screwed up my OpenGL ES calls somewhere\n");
  }
}
//////////////////////////////////////////////////////////////////////////////////////////

int main(int argc, char** argv)
{
   gf_3d_target_t      target;
   gf_display_t        gf_disp;
   EGLConfig           config;
   EGLContext          econtext;
   EGLint              num_config;
   gf_dev_info_t       info;
   gf_layer_info_t     linfo;
   gf_display_info_t   disp_info;
   GLuint              width, height;
   GLuint              it;

   /* initialize the graphics device */
   if (gf_dev_attach(&gfdev, NULL, &info)!=GF_ERR_OK)
   {
      perror("gf_dev_attach()");
      return -1;
   }

   /* Setup the layer we will use */
   if (gf_display_attach(&gf_disp, gfdev, 0, &disp_info)!=GF_ERR_OK)
   {
      fprintf(stderr, "gf_display_attach() failed\n");
      return -1;
   }

   width=disp_info.xres;
   height=disp_info.yres;

   layer_idx=disp_info.main_layer_index;

   /* get an EGL display connection */
   display=eglGetDisplay(gfdev);
   if (display==EGL_NO_DISPLAY)
   {
      fprintf(stderr, "eglGetDisplay() failed\n");
      return -1;
   }

   if (gf_layer_attach(&layer, gf_disp, layer_idx, 0)!=GF_ERR_OK)
   {
      fprintf(stderr, "gf_layer_attach() failed\n");
      return -1;
   }

   /* initialize the EGL display connection */
   if (eglInitialize(display, NULL, NULL)!=EGL_TRUE)
   {
      fprintf(stderr, "eglInitialize: error 0x%x\n", eglGetError());
      return -1;
   }

   for (it=0;; it++)
   {
      /* Walk through all possible pixel formats for this layer */
      if (gf_layer_query(layer, it, &linfo)==-1)
      {
         fprintf(stderr, "Couldn't find a compatible frame "
                         "buffer configuration on layer %d\n", layer_idx);
         return -1;
      }

      /*
       * We want the color buffer format to match the layer format,
       * so request the layer format through EGL_NATIVE_VISUAL_ID.
       */
      attribute_list[1]=linfo.format;

      /* Look for a compatible EGL frame buffer configuration */
      if (eglChooseConfig(display, attribute_list, &config, 1, &num_config)==EGL_TRUE)
      {
         if (num_config>0)
         {
            break;
         }
      }

      fprintf(stderr, "Looping\n");
   }

   /* create a 3D rendering target */
   if (gf_3d_target_create(&target, layer, NULL, 0, width, height, linfo.format)!=GF_ERR_OK)
   {
      fprintf(stderr, "Unable to create rendering target\n");
      return -1;
   }

   gf_layer_set_src_viewport(layer, 0, 0, width-1, height-1);
   gf_layer_set_dst_viewport(layer, 0, 0, width-1, height-1);
   gf_layer_enable(layer);

   /*
    * The layer settings haven't taken effect yet since we haven't
    * called gf_layer_update() yet.  This is exactly what we want,
    * since we haven't supplied a valid surface to display yet.
    * Later, the OpenGL ES library calls will call gf_layer_update()
    * internally, when  displaying the rendered 3D content.
    */

   /* create an EGL rendering context */
   econtext=eglCreateContext(display, config, EGL_NO_CONTEXT, NULL);
   if (econtext==EGL_NO_CONTEXT)
   {
      fprintf(stderr, "Create context failed: 0x%x\n", eglGetError());
      return -1;
   }

   /* create an EGL window surface */
   surface=eglCreateWindowSurface(display, config, target, NULL);
   if (surface==EGL_NO_SURFACE)
   {
      fprintf(stderr, "Create surface failed: 0x%x\n", eglGetError());
      return -1;
   }

   /* connect the context to the surface */
   if (eglMakeCurrent(display, surface, surface, econtext)==EGL_FALSE)
   {
      fprintf(stderr, "Make current failed: 0x%x\n", eglGetError());
      return -1;
   }

   init_scene(width, height);

   do {
      render_scene();
      glFinish();
      eglWaitGL();
      eglSwapBuffers(display, surface);
   } while(1);

   return 0;
}
