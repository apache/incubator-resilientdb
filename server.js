const express = require('express');
const { graphqlHTTP } = require('express-graphql');
const schema = require('./schema');

const app = express();

app.use('/graphql', graphqlHTTP({
  schema,
  graphiql: true
}));

// Default route to handle root URL
app.get('/', (req, res) => {
  res.send('Welcome to the Smart Contracts GraphQL API');
});

app.listen(4000, () => {
  console.log('Server is running on port 4000..');
});

