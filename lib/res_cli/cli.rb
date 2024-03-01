#!/usr/bin/env ruby
require 'optparse'
require 'open3'
require 'inifile'

module ResCli
  class CLI
    def self.start
      options = {}
      OptionParser.new do |opts|
        opts.banner = "Usage: #{$PROGRAM_NAME} [options]"

        opts.on('-c', '--create TYPE', [:resdb, :sdk], 'Create a new ResDB or PythonSDK instance') do |type|
          create_instance(type)
        end

        opts.on('-e', '--exec-into INSTANCE_ID', 'Bash into a running ResDB or PythonSDK instance') do |instance_id|
          exec_into(instance_id)
        end

        opts.on('-v', '--view-instances', 'View details about running instances') do
          view_instances
        end

        opts.on('-d', '--delete INSTANCE_ID', 'Delete a running ResDB or PythonSDK instance') do |instance_id|
          delete_instance(instance_id)
        end

        opts.on('-h', '--help', 'Display this help message') do
          help
          exit
        end
      end.parse!
    end

    def self.get_logged_in_user
        config['User']['Current_User']
    end

    def self.set_logged_in_user(username)
        config['User']['Current_User'] = username
        config.write    
    end

    def self.create_instance(type)
        begin
            # Implement create instance logic
            puts "Creating #{type} instance..."
        
            # Run Docker command to create a new instance
            container_name = "#{type}_instance"
            command = ["docker", "run", "--name", container_name, "-d", "expolab/#{type}:arm64"]
        
            output, status = Open3.capture2(*command)
        
            unless status.success?
                raise "Error creating instance: #{output}"
            end
        
            puts "#{type} instance created successfully with container name: #{container_name}"
        
            rescue => error
            $stderr.puts "Error creating instance: #{error}"
            end
    end

    def self.exec_into(instance_id)
        begin
            command = ["docker", "exec", "-it", instance_id, "bash"]
            Open3.check2(*command)
        
          rescue => error
            $stderr.puts "Error executing command: #{error}"
          end
    end

    def self.view_instances
        begin
            docker_command = ["docker", "container", "ls", "--format", "table {{.ID}}\t{{.Image}}\t{{.Names}}"]
            output, status = Open3.capture2(*docker_command)
        
            unless status.success?
              raise "Error running docker command: #{output}"
            end
        
            puts output
        
          rescue => error
            $stderr.puts "An unexpected error occurred: #{error}"
          end
    end

    def self.delete_instance(instance_id)
        begin
            # Implement delete instance logic
            puts "Deleting instance #{instance_id}..."
        
            # Stop the Docker container
            _, stop_status = Open3.capture2("docker stop #{instance_id}")
        
            unless stop_status.success?
              raise "Error stopping instance: #{stop_status}"
            end
        
            # Remove the Docker container
            _, rm_status = Open3.capture2("docker rm #{instance_id}")
        
            unless rm_status.success?
              raise "Error removing instance: #{rm_status}"
            end
        
            puts "Instance deleted successfully."
        
          rescue => error
            $stderr.puts "Error deleting instance: #{error}"
          end
    end

    def self.help
        puts "Usage: #{$PROGRAM_NAME} [options]"
        puts "\nOptions:"
        puts "  -c, --create TYPE     Create a new ResDB or PythonSDK instance"
        puts "  -e, --exec-into INSTANCE_ID  Bash into a running ResDB or PythonSDK instance"
        puts "  -v, --view-instances  View details about running instances"
        puts "  -d, --delete INSTANCE_ID  Delete a running ResDB or PythonSDK instance"
        puts "  -h, --help            Display this help message"
    end
  end
end

# Entry point
ResCli::CLI.start
