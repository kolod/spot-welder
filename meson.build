project('spot-welder',
  version: '1.0.0'
)

# Project configuration
board           = get_option('board')
flash_size      = get_option('flash_size')
ram_size        = get_option('ram_size')
upload_port     = get_option('upload_port')
upload_protocol = get_option('upload_protocol')

# Define board-specific configurations
board_configs = {
  'STC8H1K08': {
    'flash_size': 8192,
    'ram_size': 1024,
    'device': 'STC8H1K08',
  },
}

# Validate board selection
if board not in board_configs
  error('Unknown board: @0@'.format(board))
endif

# Apply board-specific configurations
board_config = board_configs[board]

# Find required programs
sdcc              = find_program('sdcc', required: true)
py                = find_program('python3', 'python', required: true)
print_mem_script  = files('tools/print_mem_usage.py')
detect_port_script = files('tools/detect_upload_port.py')
upload_script      = files('tools/upload_fw.py')
saved_upload_port_file = meson.current_build_dir() / 'upload-port.txt'

# Install Python dependencies at configure time.
pip_install = run_command(
  py,
  '-m', 'pip', 'install',
  '--disable-pip-version-check',
  'rich', 'stcgal', 'pyserial',
  check: false,
)

# Check if pip install succeeded
if pip_install.returncode() != 0
  error('Failed to install Python packages (rich, stcgal, pyserial) via pip at configure time:\n@0@'.format(pip_install.stderr()))
endif

# Build firmware with SDCC directly (Meson does not natively support SDCC as a C compiler).
fw = custom_target('spot-welder-fw',
  input: files('src/main.c'),
  output: 'spot-welder.ihx',
  command: [
    sdcc,
    '--model-small',
    '--std-c99',
    '-I@SOURCE_ROOT@/include',
    '-I@SOURCE_ROOT@/src',
    '-o', '@OUTPUT@',
    '@INPUT@',
  ],
  build_by_default: true,
)

custom_target('print-memory-usage',
  input: fw,
  output: 'memory-usage.txt',
  command: [
    py,
    print_mem_script,
    '@INPUT@',
    '@OUTPUT@',
    flash_size.to_string(),
    ram_size.to_string(),
  ],
  build_by_default: true,
  build_always_stale: true,
)

run_target('upload',
  command: [
    py,
    upload_script,
    '--fw', fw,
    '--protocol', upload_protocol,
    '--port-file', saved_upload_port_file,
    '--fallback-port', upload_port,
  ],
  depends: [fw],
)

run_target('detect-upload-port',
  command: [
    py,
    detect_port_script,
    '--output', saved_upload_port_file,
    '--preferred', upload_port,
  ],
)

run_target('upload-force-port',
  command: [
    py,
    '-m', 'stcgal',
    '-p', upload_port,
    '-P', upload_protocol,
    fw,
  ],
  depends: [fw],
)

# Print configuration summary
message('Board: @0@'.format(board))
message('Device: @0@'.format(board_config['device']))
message('Flash: @0@ bytes'.format(flash_size))
message('RAM: @0@ bytes'.format(ram_size))
message('Upload port: @0@'.format(upload_port))
message('Upload protocol: @0@'.format(upload_protocol))
