import { NextResponse } from 'next/server';
import solc from 'solc';

interface CompilationInput {
  language: string;
  sources: {
    [key: string]: {
      content: string;
    };
  };
  settings: {
    outputSelection: {
      [key: string]: {
        [key: string]: string[];
      };
    };
  };
}

interface CompilationOutput {
  success: boolean;
  output: string;
  abi?: any;
  bytecode?: string;
}

export async function POST(request: Request) {
  try {
    const { code } = await request.json();

    if (!code || typeof code !== 'string') {
      return NextResponse.json({
        success: false,
        output: 'Invalid contract code provided',
      });
    }

    // Configure solc input
    const input: CompilationInput = {
      language: 'Solidity',
      sources: {
        'contract.sol': {
          content: code,
        },
      },
      settings: {
        outputSelection: {
          '*': {
            '*': ['*'],
          },
        },
      },
    };

    // Compile the contract
    const output = JSON.parse(solc.compile(JSON.stringify(input)));

    // Check for errors
    if (output.errors) {
      const errors = output.errors
        .filter((error: any) => error.severity === 'error')
        .map((error: any) => error.formattedMessage);
      
      if (errors.length > 0) {
        return NextResponse.json({
          success: false,
          output: errors.join('\n'),
        });
      }
    }

    // Get the contract name (assuming it's the first contract in the file)
    const contractName = Object.keys(output.contracts['contract.sol'])[0];
    const contract = output.contracts['contract.sol'][contractName];

    if (!contract) {
      return NextResponse.json({
        success: false,
        output: 'No contract found in the provided code',
      });
    }

    const response: CompilationOutput = {
      success: true,
      output: 'Contract compiled successfully!',
      abi: contract.abi,
      bytecode: contract.evm.bytecode.object,
    };

    return NextResponse.json(response);
  } catch (error) {
    console.error('Compilation error:', error);
    return NextResponse.json({
      success: false,
      output: 'Failed to compile contract. Please check your code and try again.',
    });
  }
} 