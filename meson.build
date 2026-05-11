project('spot-welder',
  version: '1.0.0'
)

# Project configuration
board = get_option('board')
flash_size = get_option('flash_size')
ram_size = get_option('ram_size')

# Define board-specific configurations
board_configs = {
  'STC8H1K08': {
    'flash_size': 8192,
    'ram_size': 1024,
    'device': 'STC8H1K08',
  },
}

if board not in board_configs
  error('Unknown board: @0@'.format(board))
endif

board_config = board_configs[board]

sdcc = find_program('sdcc', required: true)
py = find_program('python3', 'python', required: true)
print_mem_script = files('tools/print_mem_usage.py')

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

# Print configuration summary
message('Board: @0@'.format(board))
message('Device: @0@'.format(board_config['device']))
message('Flash: @0@ bytes'.format(flash_size))
message('RAM: @0@ bytes'.format(ram_size))
