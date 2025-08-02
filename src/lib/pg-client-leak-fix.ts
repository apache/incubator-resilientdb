// Runtime patch to prevent MaxListenersExceededWarning on pg Client instances
// Inspired by understanding of EventEmitter leak warnings and pg pool behaviour.
// The pool attaches a new 'end' listener to the same client every time it is
// checked out with pool.connect(). Over time this exceeds the default limit (10)
// and Node prints a warning. We keep only the most recent listener in place.

import chalk from "chalk";
import { EventEmitter } from "events";
import { Client, Pool } from "pg";

// Patch Pool.prototype.connect so that after a client is checked out we
// de-duplicate the 'end' listeners that have accumulated on the client.
const originalConnect: (this: Pool, ...args: any[]) => Promise<Client> = (Pool as any).prototype.connect;

(Pool as any).prototype.connect = async function patchedConnect(this: Pool, ...args: any[]): Promise<Client> {
  const client: Client = await originalConnect.apply(this, args);

  // Only touch EventEmitter listeners if we are dealing with a pg Client
  // and there is more than one 'end' listener attached.
  const endListeners = (client as unknown as EventEmitter)?.listeners("end") || [];
  if (endListeners.length > 1) {
    // Keep the most recently attached listener (the last in the array) and
    // remove the earlier ones. This preserves pool's current release handler
    // while discarding the obsolete duplicates responsible for the warning.
    const listenersToRemove = endListeners.slice(0, -1);
    listenersToRemove.forEach((l) => {
      (client as unknown as EventEmitter).removeListener("end", l as (...args: any[]) => void);
    });

    // Optional debug logging – comment out if noise is undesirable.
    /* eslint-disable no-console */
    console.log(
      chalk.gray(
        `[pg-client-leak-fix] Removed ${listenersToRemove.length} duplicate 'end' listener${
          listenersToRemove.length === 1 ? "" : "s"
        } from pg Client`
      )
    );
    /* eslint-enable no-console */
  }

  return client;
};
