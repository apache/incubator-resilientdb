import express from 'express';
import { graphqlHTTP } from 'express-graphql';
import schema from './schema.js';
import logger from './logger.js';

const app = express();

app.use('/graphql', graphqlHTTP({
  schema,
  graphiql: true,
}));

app.get('/', (req, res) => {
  res.send('Welcome to the Smart Contracts GraphQL API');
});

app.use((err, req, res, next) => {
  logger.error(`Express error: ${err.message}`);
  res.status(500).send('Internal Server Error');
});

const PORT = process.env.PORT || 8400;

app.listen(PORT, () => {
  console.log(`Server is running on port ${PORT}...`);
});
