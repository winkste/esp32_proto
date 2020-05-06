# Warning! This code was automatically generated for projects
# without default 'CMakeLists.txt' file.

set(app_sources
	"appIdent.c"
	"controlTask.c"
	"logcfg.c"
	"mymain.c"
	"otaUpdate.c"
)

idf_component_register(SRCS ${app_sources})
