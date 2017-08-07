#!/usr/bin/env ruby

# This is a simple test framework which runs tests specified as JSON files in
# the tests/ directory. The format of the tests is:
#
# {
#   "source": "<the source file contents>",
#   "stdout": "<the expected STDOUT>",
#   "stderr": "<the expected STDERR>",
#   "exit_code": <the expected exit code>
# }
#
# Run ./test.rb with no arguments for more information. This script is meant to
# be run from the root of the repository (via `make test`), not from this
# directory.

require 'colorize'
require 'json'
require 'open3'
require 'tempfile'
require 'thor'

class PoiTest < Thor
  class_option :binary,
    :default => 'poi',
    :desc => 'The path to the Poi binary.'

  desc 'test [TEST]', 'Run a specific test or all the tests.'
  def test(name=nil)
    if name
      if !run_test(name)
        exit(1)
      end
    else
      total = 0
      passed = 0
      Dir['tests/*.json'].each do |path|
        total += 1
        if run_test(path.split('/').last.chomp('.json'))
          passed += 1
        end
        puts('')
      end
      puts("Total: #{total}")
      puts("Passed: #{passed}")
      puts("Failed: #{total - passed}")
      if passed != total
        exit(1)
      end
    end
  end

private

  def run_test(name)
    puts("Running test: #{name}".blue)
    path = "tests/#{name}.json"
    begin
      test_info_json = File.read(path)
    rescue Errno::ENOENT
      puts("Error: Unable to open file '#{path}'".red)
      return false
    end
    test_info = JSON.parse(test_info_json)
    source_file = Tempfile.new('poi')
    source_file.write(test_info['source'])
    source_file.close
    begin
      stdout, stderr, status = Open3.capture3(
        options[:binary],
        source_file.path
      )
    rescue Errno::ENOENT
      puts("Unable to run binary: #{options[:binary]}".red)
      return false
    end
    source_file.unlink
    if stdout != test_info['stdout']
      puts("Expected STDOUT: #{test_info['stdout'].inspect}".red)
      puts("Got: #{stdout.inspect}")
      return false
    end
    if stderr != test_info['stderr']
      puts("Expected STDERR: #{test_info['stderr'].inspect}".red)
      puts("Got: #{stderr.inspect}")
      return false
    end
    if status.exitstatus != test_info['exit_code']
      puts("Expected exit code: #{test_info['exit_code']}".red)
      puts("Got: #{status.exitstatus}".red)
      return false
    end
    puts('OK'.green)
    return true
  end

end

PoiTest.start(ARGV)
