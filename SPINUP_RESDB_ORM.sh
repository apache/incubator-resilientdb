# TODO: Change this to 300 (5 minutes) once done testing
max_iterator=500

touch ormSpinup.log
./service/tools/kv/server_tools/start_kv_service.sh 2>> ormSpinup.log &

iterator=0
while ! grep "nohup: redirecting stderr to stdout|Build completed successfully" ormSpinup.log; do
    sleep 1
    iterator=$((iterator + 1))
    if [ $iterator -gt $max_iterator ]; then
        echo "Timed out waiting for KV service to start"
        echo "Run \`./service/tools/kv/server_tools/start_kv_service.sh\` yourself to manually diagnose errors"
        # TODO: Un remove this once you diagnose the problem
        # rm ormSpinup.log
        return 1
    fi
done

# Just in case the service needs a minute
sleep 2

echo "KV service started successfully, now starting GraphQL service"
rm ormSpinup.log
touch ormSpinup.log

cd ecosystem/graphql
bazel-bin/service/http_server/crow_service_main ./ecosystem/graphql/service/tools/config/interface/service.config ./ecosystem/graphql/service/http_server/server_config.config > ormSpinup.log 2>&1 &

# TODO: Delete this
sleep 5

iterator=0
while ! grep "[INFO    ]" ormSpinup.log; do
    sleep 1
    iterator=$((iterator + 1))
    if [ $iterator -gt $max_iterator ]; then
        echo "Timed out waiting for GraphQL service to start"
        echo "Note that the kv service is currently running"
        echo "Run \`bazel-bin/service/http_server/crow_service_main ./ecosystem/graphql/service/tools/config/interface/service.config ./ecosystem/graphql/service/http_server/server_config.config > ormSpinup.log 2>&1 &\` yourself to manually diagnose errors"
        rm ormSpinup.log
        return 1
    fi
done

rm ormSpinup.log
# We wait for the SECOND [INFO] message
sleep 2

echo "kv_service and graphql_service started successfully"
echo "It should be safe to enable your venv and use ResDB-orm now"

# TODO: GitHub Copilot installed itself without asking, and suggested the following line. It might actually work for pkill (??)
# echo "to stop them, run: ./service/tools/kv/server_tools/stop_kv_service.sh and pkill crow_service_main"