const { exec } = require("child_process");

exec("ps aux | grep kv_service | grep -v grep | head -n 1 | awk '{print $1}'", (err, stdout, stderr) => {
  if (err) {
    console.error(`Error finding kv_service process: ${stderr}`);
    return;
  }

  const pid = stdout.trim();

  if (pid) {
    console.log("PID of CPP Client-1", pid)
    const server_addr = process.env.PYROSCOPE_SERVER_ADDRESS  || "http://localhost:4040"
    const command = `pyroscope connect --server-address ${server_addr} --application-name cpp_client_1 --spy-name ebpfspy --pid ${pid}`;
    exec(command, (err, stdout, stderr) => {
      if (err) {
        console.error(`Error running pyroscope connect: ${stderr}`);
        return;
      }
      console.log(`Pyroscope connected successfully:\n${stdout}`);
    });
  } else {
    console.log("kv_service process not found.");
  }
});