
project('timer', 'c')

compiler = meson.get_compiler('c')
deps = []
sources = ['src/timer.c']

# FIXME: has_argument doesn't seem to work here for conditionally adding this flag only for the MSVC compiler
# FIXME: is this required for dynamic builds?
#if compiler.has_argument('/NODEFAULTLIB:libcmt')
add_project_link_arguments('/NODEFAULTLIB:libcmt', language: 'c')
#endif

os = host_machine.system()
arch = host_machine.cpu_family()
win_module = import('windows')

if os == 'windows'
	deps += compiler.find_library('winmm')
	deps += compiler.find_library('comctl32')
	if compiler.has_argument('-mwindows')
		add_project_arguments('-mwindows', language: 'c')
		add_project_link_arguments('-mwindows', language: 'c')
	endif

	if arch == 'x86'
		sources += win_module.compile_resources('res/resource32.rc')
	elif arch == 'x86_64'
		sources += win_module.compile_resources('res/resource64.rc')
	endif

endif

configure_file(input: 'res/timer.ico', output: 'timer.ico', copy: true)
configure_file(input: 'res/alarm.wav', output: 'alarm.wav', copy: true)

deps += compiler.find_library('iup')

executable('timer', sources, dependencies: deps, gui_app: true)
