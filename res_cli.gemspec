Gem::Specification.new do |spec|
    spec.name          = 'res_cli'
    spec.version       = '0.1.0'
    spec.authors       = ['gopuman']
    spec.email         = ['gopalnambiar2@gmail.com']
    spec.summary       = 'CLI to manage ResilientDB and Python SDK instances'
    spec.description   = 'A command-line interface (CLI) to manage ResilientDB and Python SDK instances.'    
    spec.homepage      = 'https://github.com/ResilientApp/ResCLI'
    spec.license       = 'APSL-2.0'
  
    spec.files         = Dir['lib/**/*', 'bin/*', 'config.ini']
    spec.executables   = ['res_cli']
    spec.require_paths = ['lib']
  
    spec.add_runtime_dependency 'open3', '~> 0.2.1'
    spec.add_runtime_dependency 'optparse', '~> 0.4.0'
    spec.add_runtime_dependency 'inifile', '~> 3.0.0'
end
  