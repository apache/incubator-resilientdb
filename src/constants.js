export const OPS = {
    CREATE: "CREATE",
    FETCH: "FETCH",
    UPDATE: "UPDATE",
    UPDATE_ALL: "UPDATE MULTIPLE",
    FILTER: "FILTER",
    INFO: "ACCOUNT"
};

export const OP_DATA = {
    [OPS.CREATE]: {
        from: "commit",
        requiredFields: ['address', 'amount', 'data']
    },
    [OPS.FETCH]: {
        from: "get",
        requiredFields: ['id']
    },
    [OPS.UPDATE]: {
        from: "update",
        requiredFields: ['address', 'amount', 'data', 'id']
    },
    [OPS.UPDATE_ALL]: {
        from: "update-multi",
        requiredFields: ['values']
    },
    [OPS.FILTER]: {
        from: "filter",
        requiredFields: ['recipientPublicKey']
    },
    [OPS.INFO]: {
        from: "account",
        requiredFields: []
    }
}
