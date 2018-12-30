// Sources:
// http://www.opengl-tutorial.org/beginners-tutorials/tutorial-2-the-first-triangle/
// https://stackoverflow.com/questions/13403807/glvertexattribpointer-raising-gl-invalid-operation

#define GL_GLEXT_PROTOTYPES 1

#include <GL/gl.h>
#include <gtk/gtk.h>

static GLfloat vertices[] = {
  -1.0f, -1.0f, -1.0f,
   1.0f, -1.0f, -1.0f,
   0.0f,  1.0f, -1.0f,
};

static GLuint program;
static GLuint vao;
static GLuint buffer;

static GLuint load_shaders() {
	GLuint vertex_shader = glCreateShader(GL_VERTEX_SHADER);
	GLuint fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);

	const char* vertex_shader_code = "\
    #version 330 core\n \
    layout(location = 0) in vec3 vertexPosition_modelspace; \n \
    void main() { \n \
      gl_Position.xyz = vertexPosition_modelspace; \n \
      gl_Position.w = 1.0; \n \
    }";

  const char* fragment_shader_code = "\
    #version 330 core \n \
    out vec3 color; \n \
    void main() { \n \
      color = vec3(1, 0, 0); \n \
    }";

	GLint result = GL_FALSE;

	glShaderSource(vertex_shader, 1, &vertex_shader_code, NULL);
	glCompileShader(vertex_shader);
	glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &result);
	if (!result) {
    goto fail;
	}

	glShaderSource(fragment_shader, 1, &fragment_shader_code , NULL);
	glCompileShader(fragment_shader);
	glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &result);
	if (!result) {
		goto fail;
	}

	GLuint program = glCreateProgram();
	glAttachShader(program, vertex_shader);
	glAttachShader(program, fragment_shader);
	glLinkProgram(program);
	glGetProgramiv(program, GL_LINK_STATUS, &result);
	if (!result) {
		goto fail;
	}

	glDetachShader(program, vertex_shader);
	glDetachShader(program, fragment_shader);
	glDeleteShader(vertex_shader);
	glDeleteShader(fragment_shader);

	return program;

fail:
  printf("failed to create shaders.\n");
  exit(1);
}

static gboolean render(GtkGLArea* area, GdkGLContext* ctx) {
  glClearColor(0, 0, 0, 0);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  glUseProgram(program);

  glEnableVertexAttribArray(0);
  glBindBuffer(GL_ARRAY_BUFFER, buffer);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, NULL);
  glDrawArrays(GL_TRIANGLES, 0, 3);
  glDisableVertexAttribArray(0);

  return TRUE;
}

static void realize(GtkGLArea *glarea) {
	gtk_gl_area_make_current(glarea);
  gtk_gl_area_set_has_depth_buffer(glarea, TRUE);

  program = load_shaders();

  glGenVertexArrays(1, &vao);
  glBindVertexArray(vao);

  glGenBuffers(1, &buffer);
  glBindBuffer(GL_ARRAY_BUFFER, buffer);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
}

static void activate(GtkApplication* app, gpointer userData) {
  GtkWidget* window = gtk_application_window_new(app);
  gtk_window_set_title(GTK_WINDOW(window), "GL Demo");
  gtk_window_set_default_size(GTK_WINDOW(window), 400, 400);

  GtkWidget* gl_area = gtk_gl_area_new();
  g_signal_connect(gl_area, "render", G_CALLBACK(render), NULL);
  g_signal_connect(gl_area, "realize", G_CALLBACK(realize), NULL);
  gtk_container_add(GTK_CONTAINER(window), gl_area);

  gtk_widget_show_all(window);
}

int main(int argc, char** argv) {
  GtkApplication* app = gtk_application_new("com.aqnichol.button_catcher",
                                            G_APPLICATION_FLAGS_NONE);
  g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
  int status = g_application_run(G_APPLICATION(app), argc, argv);
  g_object_unref(app);
  return status;
}
