# INSTRUCTIONS TO USE DEV MODE

- Click on dashboard on the landing page and navigate to this page: (Execution View)[https://reslens.resilientdb.com/cpu]

- Open the ResLens Navigation and toggle the mode on the top right corner to dev. This will bring up the UI for the development instance of ResilientDB with debug information.

![ResLens Development Mode](/docs/screenshots/reslens_dev_mode.png)

- Check the tour option and select `Development Tour` to highlight all the features available at your disposal to seed data and check the flamegraph. Make sure to open the utility tools section to get a tour of each component.

- Click the Fire SetValues button which will seed 1000 (default) random key value pairs into the database. This simulates a long running process i.e (> 30s) and enlists function calls.

![ResLens Development Fire SetValues Button](/docs/screenshots/reslens_dev_fire_setvalues.png)

- Please do not spam this button. Use it once to twice according to requirement. Hitting the function once should populate the flamegraph.

- Once executed, let the seeding process carry on, a progress bar will indicate the progress.

![ResLens Flamegraph Empty Searchbar](/docs/screenshots/reslens_dev_fg_sb.png)

- Use the refresh button on the flamegraph to refresh the flamegraph state to view the data. Note once the function is run, flamegraph data will be populated with a latency of 30 - 45s. Indicated by the red arrow below in the screenshot

![ResLens Flamegraph Complete Searchbar with Refresh](/docs/screenshots/reslens_dev_search_refresh.png)

- Here is what you can search on the search bar to inspect the callgraph. Indicated as the value typed in the search bar. Switch the mode to `sandwich` to inspect the function more closely.

![ResLens Flamegraph Sandwich mode](/docs/screenshots/reslens_dev_sandwich_menu.png)

- Inspect the function in more detail like so, by clicking on the function. 

![ResLens Flamegraph Inspect](/docs/screenshots/reslens_dev_insepect.png)