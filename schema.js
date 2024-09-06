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
      args: { path: { type: GraphQLString } },
      resolve(parent, args) {
        return compileContract(args.path);
      }
    },
    deployContract: {
      type: GraphQLString,
      args: { path: { type: GraphQLString } },
      resolve(parent, args) {
        return deployContract(args.path);
      }
    },
    executeContract: {
      type: GraphQLString,
      args: { command: { type: GraphQLString } },
      resolve(parent, args) {
        return executeContract(args.command);
      }
    }
  }
});

module.exports = new GraphQLSchema({
  query: RootQuery
});

