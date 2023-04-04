Run Node Manager as backend to lauch Nexres locally. 

If you are under the repo root path, go the deploy/node_manager.
Then
```
bazel run :deploy_manager $PWD
```

Launch your front end from : [resilientdb](https://github.com/resilientdb/resilientdb.github.io/tree/nexres)
Change the [endpoint address](https://github.com/resilientdb/resilientdb.github.io/blob/nexres/src/api/endpoint.ts#L22) so all the deployment request will be sent to your node manager backend.
