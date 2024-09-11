const { GraphQLObjectType, GraphQLSchema, GraphQLString } = require('graphql');
const { createAccount, compileContract, deployContract, executeContract } = require('./cli-functions');

const RootQuery = new GraphQLObjectType({
  name: 'RootQueryType',
  fields: {
    createAccount: {
      type: GraphQLString,
      args: { config: { type: GraphQLString } },
      resolve(parent, args) {
        return createAccount(args.config);
      }
    },
    compileContract: {
      type: GraphQLString,
      args: {
        sourcePath: { type: GraphQLString },
        outputPath: { type: GraphQLString }
      },
      resolve(parent, args) {
        return compileContract(args.sourcePath, args.outputPath);
      }
    },
    deployContract: {
      type: GraphQLString,
      args: {
        config: { type: GraphQLString },
        contract: { type: GraphQLString },
        name: { type: GraphQLString },
        arguments: { type: GraphQLString },
        owner: { type: GraphQLString }
      },
      resolve(parent, args) {
        return deployContract(args.config, args.contract, args.name, args.arguments, args.owner);
      }
    },
    executeContract: {
      type: GraphQLString,
      args: {
        config: { type: GraphQLString },
        sender: { type: GraphQLString },
        contract: { type: GraphQLString },
        function: { type: GraphQLString },
        arguments: { type: GraphQLString }
      },
      resolve(parent, args) {
        return executeContract(args.config, args.sender, args.contract, args.function, args.arguments);
      }
    }
  }
});

module.exports = new GraphQLSchema({
  query: RootQuery
});
