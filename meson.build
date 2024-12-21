project('astroterm', 'c',
    default_options : ['warning_level=3', 'c_std=c99', 'buildtype=debugoptimized'],
    meson_version: '>=1.4.0',
)

# ------------------------------------------------------------------------------
# Dependencies
# ------------------------------------------------------------------------------

conf = configuration_data()

# Build submodules
project_dependencies = []
project_source_files = []
project_header_files = []
project_include_dirs = [
    include_directories('include'),
    include_directories('data'),
    include_directories('third_party'),
]

subdir('include')
subdir('src')
subdir('data')
subdir('third_party')

# Get dependencies
cc = meson.get_compiler('c')
curses = dependency('curses', required : true)
c_stdlib = dependency('c', required: true)
math = cc.find_library('m', required : true)
project_dependencies += [curses, c_stdlib, math]

# ------------------------------------------------------------------------------
# Curses info
# ------------------------------------------------------------------------------

# Define some curses preprocessor symbols
if curses.found()
    check_headers = [
        ['ncursesw/menu.h',     'HAVE_NCURSESW_MENU_H'],
        ['ncurses/menu.h',      'HAVE_NCURSES_MENU_H'],
        ['menu.h',              'HAVE_MENU_H'],
        ['ncursesw/curses.h',   'HAVE_NCURSESW_CURSES_H'],
        ['ncursesw.h',          'HAVE_NCURSESW_H'],
        ['ncurses/curses.h',    'HAVE_NCURSES_CURSES_H'],
        ['ncurses.h',           'HAVE_NCURSES_H'],
        ['curses.h',            'HAVE_CURSES_H'],
    ]
    foreach h : check_headers
        if cc.has_header(h.get(0))
            conf.set(h.get(1), 1)
        endif
    endforeach
endif

# Check what curses headers are available
if (conf.get('HAVE_NCURSESW_H', 0) == 1)
    # Have ncurses with wide char support
else
    if (conf.get('HAVE_NCURSES_H', 0) == 1)
        # Have ncurses without wide char support*
        # *on certain systems, the ncurses header includes wide char functions
        warning('ncursesw header not found, which is sometimes needed for ' + 
                'unicode support. This may not be an issue as the regular ' +
                'ncurses.h sometimes contains wide character support')
    else
        if (conf.get('HAVE_CURSES_H', 0) == 1)
            # Have curses but not ncurses 
            warning('ncurses header not found. Ncurses is required for ' +
                    'certain features such as color support')
        endif
    endif
endif

# ------------------------------------------------------------------------------
# Embed data
# ------------------------------------------------------------------------------

bsc5_data = configure_file(
    input: 'data/bsc5',
    output: 'bsc5_data.h',
    command: [
        'sh', '../scripts/embed.sh', '@INPUT@', '@OUTPUT@'
    ]
)
bsc5_constellations = configure_file(
    input: 'data/bsc5_constellations.txt',
    output: 'bsc5_constellations.h',
    command: [
        'sh', '../scripts/embed.sh', '@INPUT@', '@OUTPUT@'
    ]
)
bsc5_names = configure_file(
    input: 'data/bsc5_names.txt',
    output: 'bsc5_names.h',
    command: [
        'sh', '../scripts/embed.sh', '@INPUT@', '@OUTPUT@'
    ]
)
project_source_files += [bsc5_data, bsc5_constellations, bsc5_names]

# ------------------------------------------------------------------------------
# Application executable
# ------------------------------------------------------------------------------

executable(
    meson.project_name(),
    project_source_files + 'src/main.c',
    dependencies        : project_dependencies,
    include_directories : project_include_dirs,
    c_args              : ['-Wall', '-Wextra', '-Wno-unused-variable', '-Wno-unused-parameter', '-g'],
    install             : true,
)

# ------------------------------------------------------------------------------
# Testing
# ------------------------------------------------------------------------------

test_files = []
test_infra_source_files = []
test_include_dirs = []
subdir('test')

# Compile test modules separately (this keeps things a bit cleaner, can call
# main individually in each file.
foreach test_file : test_files
    filepath = test_file[0].full_path()
    filename = filepath.split('/')[-1]
    test_name = filename.split('.')[0]
    test_exe = executable(
        test_name,
        [test_file] + project_source_files + test_infra_source_files,
        dependencies: project_dependencies,
        include_directories: project_include_dirs + test_include_dirs,
        c_args: ['-Wall', '-Wextra', '-Wno-unused-variable', '-Wno-unused-parameter', '-g'],
        install: false
    )

    test(test_name, test_exe)
endforeach
