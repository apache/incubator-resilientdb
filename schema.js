import {
  GraphQLObjectType,
  GraphQLSchema,
  GraphQLString,
  GraphQLNonNull,
  GraphQLBoolean,
} from 'graphql';
import { createAccount, addAddress, compileContract, deployContract, executeContract } from './cli-functions.js';

const RootQuery = new GraphQLObjectType({
  name: 'Query',
  fields: {
    _empty: {
      type: GraphQLString,
      resolve() {
        return 'This is a placeholder query.';
      },
    },
  },
});

const DeployContractOutput = new GraphQLObjectType({
  name: 'DeployContractOutput',
  fields: {
    ownerAddress: { type: GraphQLString },
    contractAddress: { type: GraphQLString },
    contractName: { type: GraphQLString },
  },
});

const RootMutation = new GraphQLObjectType({
  name: 'Mutation',
  fields: {
    createAccount: {
      type: GraphQLString,
      args: {
        config: { type: new GraphQLNonNull(GraphQLString) },
	type: { type: GraphQLString },
      },
      async resolve(parent, args) {
        try {
          const result = await createAccount(args.config, args.type);
          return result;
        } catch (error) {
          throw new Error(error);
        }
      },
    },
    addAddress: {
      type: GraphQLString,
      args: {
        config: { type: new GraphQLNonNull(GraphQLString) },
        address: { type: new GraphQLNonNull(GraphQLString) },
        type: { type: GraphQLString },
      },
      async resolve(parent, args) {
        try {
          const result = await addAddress(args.config, args.address, args.type);
          return result;
        } catch (error) {
          throw new Error(error);
        }
      },
    },
    compileContract: {
      type: GraphQLString,
      args: {
        source: { type: new GraphQLNonNull(GraphQLString) },
        type: { type: GraphQLString }, 
      },
      async resolve(parent, args) {
        try {
          const result = await compileContract(args.source, args.type);
          return result;
        } catch (error) {
          throw new Error(error);
        }
      },
    },
    deployContract: {
      type: DeployContractOutput,
      args: {
        config: { type: new GraphQLNonNull(GraphQLString) },
        contract: { type: new GraphQLNonNull(GraphQLString) },
        name: { type: new GraphQLNonNull(GraphQLString) },
        arguments: { type: new GraphQLNonNull(GraphQLString) },
        owner: { type: new GraphQLNonNull(GraphQLString) },
	type: { type: GraphQLString },
      },
      async resolve(parent, args) {
        try {
          const result = await deployContract(
            args.config,
            args.contract,
            args.name,
            args.arguments,
            args.owner,
	    args.type
          );
          return result;
        } catch (error) {
          throw new Error(error);
        }
      },
    },
    executeContract: {
      type: GraphQLString,
      args: {
        config: { type: new GraphQLNonNull(GraphQLString) },
        sender: { type: new GraphQLNonNull(GraphQLString) },
        contract: { type: new GraphQLNonNull(GraphQLString) },
        functionName: { type: new GraphQLNonNull(GraphQLString) },
        arguments: { type: new GraphQLNonNull(GraphQLString) },
	type: { type: GraphQLString },
      },
      async resolve(parent, args) {
        try {
          const result = await executeContract(
            args.config,
            args.sender,
            args.contract,
            args.functionName,
            args.arguments,
            args.type
          );
          return result;
        } catch (error) {
          throw new Error(error);
        }
      },
    },
  },
});

export default new GraphQLSchema({
  query: RootQuery,
  mutation: RootMutation,
});
