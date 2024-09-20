const {
  GraphQLObjectType,
  GraphQLSchema,
  GraphQLString,
  GraphQLNonNull,
} = require('graphql');
const {
  createAccount,
  compileContract,
  deployContract,
  executeContract,
} = require('./cli-functions');

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

const RootMutation = new GraphQLObjectType({
  name: 'Mutation',
  fields: {
    createAccount: {
      type: GraphQLString,
      args: {
        config: { type: new GraphQLNonNull(GraphQLString) },
      },
      async resolve(parent, args) {
        try {
          const result = await createAccount(args.config);
          return result;
        } catch (error) {
          throw new Error(error);
        }
      },
    },
    compileContract: {
      type: GraphQLString,
      args: {
        sourcePath: { type: new GraphQLNonNull(GraphQLString) },
        outputPath: { type: new GraphQLNonNull(GraphQLString) },
      },
      async resolve(parent, args) {
        try {
          const result = await compileContract(args.sourcePath, args.outputPath);
          return result;
        } catch (error) {
          throw new Error(error);
        }
      },
    },
    deployContract: {
      type: GraphQLString,
      args: {
        config: { type: new GraphQLNonNull(GraphQLString) },
        contract: { type: new GraphQLNonNull(GraphQLString) },
        name: { type: new GraphQLNonNull(GraphQLString) },
        arguments: { type: new GraphQLNonNull(GraphQLString) },
        owner: { type: new GraphQLNonNull(GraphQLString) },
      },
      async resolve(parent, args) {
        try {
          const result = await deployContract(
            args.config,
            args.contract,
            args.name,
            args.arguments,
            args.owner
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
      },
      async resolve(parent, args) {
        try {
          const result = await executeContract(
            args.config,
            args.sender,
            args.contract,
            args.functionName,
            args.arguments
          );
          return result;
        } catch (error) {
          throw new Error(error);
        }
      },
    },
  },
});

module.exports = new GraphQLSchema({
  query: RootQuery,
  mutation: RootMutation,
});

