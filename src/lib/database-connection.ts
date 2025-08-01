import chalk from "chalk";
import { Pool, PoolClient } from "pg";
import { config } from "../config/environment";
import "./pg-client-leak-fix";

class DatabaseConnection {
  private static instance: DatabaseConnection;
  private pool: Pool | null = null;

  private constructor() {}

  static getInstance(): DatabaseConnection {
    if (!DatabaseConnection.instance) {
      DatabaseConnection.instance = new DatabaseConnection();
    }
    return DatabaseConnection.instance;
  }

  private createPool(): Pool {
    if (this.pool) {
      return this.pool;
    }

    console.log(chalk.blue("[DatabaseConnection] Creating PostgreSQL connection pool..."));

    this.pool = new Pool({
      connectionString: config.databaseUrl,
      ssl: process.env.NODE_ENV === "production" ? { rejectUnauthorized: false } : false,
      max: 10, // Maximum number of clients in the pool
      idleTimeoutMillis: 30000, // How long a client is allowed to remain idle
      connectionTimeoutMillis: 2000, // How long to wait for a connection
    });

    // Handle pool errors
    this.pool.on('error', (err) => {
      console.error(chalk.red('[DatabaseConnection] Unexpected error on idle client'), err);
    });

    console.log(chalk.green("[DatabaseConnection] PostgreSQL connection pool created"));
    return this.pool;
  }

  async getClient(): Promise<PoolClient> {
    const pool = this.createPool();
    return await pool.connect();
  }

  async query(text: string, params?: any[]): Promise<any> {
    const pool = this.createPool();
    return await pool.query(text, params);
  }

  async close(): Promise<void> {
    if (this.pool) {
      console.log(chalk.yellow("[DatabaseConnection] Closing connection pool..."));
      await this.pool.end();
      this.pool = null;
      console.log(chalk.green("[DatabaseConnection] Connection pool closed"));
    }
  }

  async healthCheck(): Promise<boolean> {
    try {
      const result = await this.query('SELECT 1 as health');
      return result.rows.length > 0 && result.rows[0].health === 1;
    } catch (error) {
      console.error(chalk.red("[DatabaseConnection] Health check failed:"), error);
      return false;
    }
  }
}

// Export singleton instance
export const databaseConnection = DatabaseConnection.getInstance();