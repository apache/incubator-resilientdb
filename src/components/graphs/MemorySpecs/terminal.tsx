//@ts-nocheck
import { Card, CardContent, CardHeader, CardTitle } from "@/components/ui/card";
import { middlewareApi } from "@/lib/api";
import { SquareTerminal } from "lucide-react";
import { useState } from "react";
import Terminal, { ColorMode, TerminalOutput } from "react-terminal-ui";

export const TerminalController = (props = {}) => {
  const [terminalLineData, setTerminalLineData] = useState([
    <TerminalOutput>Welcome to ResilientDB Playground!</TerminalOutput>,
  ]);

  const processCommand = async (command: string) => {
    const args = command.split(" ");
    let output: JSX.Element;
    console.log(args);

    if (args[0] === "resdb") {
      if (args[1] === "set" && args[2] === "--key" && args[4] === "--value") {
        console.log("here");
        const key = args[3].replace(/"/g, "");
        const value = args[5].replace(/"/g, "");
        try {
          const response = await middlewareApi.post("/transactions/set", {
            id: key,
            value,
          });
          if (response?.status !== 200) {
            output = (
              <p>
                Error: Unable to set key {key} to {value}
              </p>
            );
            return;
          }
          output = (
            <p>
              Successfully set {key} to {value}
            </p>
          );
        } catch (error) {
          output = (
            <p>
              Error: Unable to set key {key} to {value}
            </p>
          );
        }
      } else if (args[1] === "get" && args[2] === "--key") {
        const key = args[3].replace(/"/g, "");
        try {
          const response = await middlewareApi.get(`/transactions/get/${key}`);
          if (response?.status !== 200) {
            output = <p>Error: Unable to get value for {key}</p>;
            return;
          }
          let value = response?.data?.value;
          if (!value) {
            value = "No value found for key";
          }
          output = (
            <p>
              Value for {key}: {String(value)}
            </p>
          );
        } catch (error) {
          output = <p>Error: Unable to get value for {key}</p>;
        }
      } else {
        output = (
          <TerminalOutput>
            Invalid command. Use "redb set --key "key" --value "value"" or "redb
            get --key "key""
          </TerminalOutput>
        );
      }
    } else if (command === "help") {
      output = (
        <TerminalOutput>
          Available commands:
          <br />
          redb set --key "key" --value "value" : Set a key-value pair
          <br />
          redb get --key "key" : Get the value for a key
          <br />
        </TerminalOutput>
      );
    } else if (command === "clear") {
      setTerminalLineData([]);
      return;
    } else {
      output = (
        <TerminalOutput>
          Unknown command. Type 'help' for available commands.
        </TerminalOutput>
      );
    }

    setTerminalLineData((prev) => [
      ...prev,
      <TerminalOutput>$ {command}</TerminalOutput>,
      output,
    ]);
  };

  return (
    <Card>
      <CardHeader>
        <div className="flex items-center gap-2">
          <SquareTerminal className="w-6 h-6 text-blue-400" />
          <CardTitle className="text-2xl font-bold">Playground</CardTitle>
        </div>
      </CardHeader>
      <CardContent>
        <div className="h-fit bg-inherit">
          <Terminal
            key="terminal"
            colorMode={ColorMode.Dark}
            height="300px"
            onInput={processCommand}
          >
            {terminalLineData}
          </Terminal>
        </div>
      </CardContent>
    </Card>
  );
};
