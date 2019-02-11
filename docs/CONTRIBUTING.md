If you like PhoenixGo and have some ideas of how to improve it, you can ask to 
merge your ideas into the PhoenixGo page

All the steps below need to use the ubuntu/linux terminal in some steps, but all 
the steps below are also possible on windows and mac

To contribute to PhoenixGo, you need to : 

### 1) fork Tencent/PhoenixGo master branch

go on the github [here](https://github.com/Tencent/PhoenixGo) , 
click on "fork" on the top right

PhoenixGo will be forked to your github account and added to your respositories

### 2) clone your forked PhoenixGo master branch

in a terminal, do :

```
git clone -b master https://github.com/yourgithubusername/PhoenixGo/
```

you will see a warning message but dont mind it

A local copy of your PhoenixGo fork will be cloned/downloaded on your computer

### 3) (optional) update your fork if it is outdated

this step is needed only if you forked from Tencent/PhoenixGo master branch 
long ago : there are probably new commits available

The code below will : 

- add an entry to the "upstream" parent project source
- add a link of the parent upstream project in upstream/master (your fork is in origin/master)
- pull the github server latest "upstream" (Tencent parent source) changes to your 
local branch (your fork)
- check which branch it is to make sure we didnt do a mistake (we are updating master 
so it has to be master)
- push your local changes to yourgithubusername github website and server (you updated
 your old fork master with the latest master "upstream" master changes), username and 
password are asked for security, provide them and press ENTER to confirm

to update your fork to latest Tencent commits, do :

```
git remote add upstream https://github.com/Tencent/PhoenixGo && git fetch upstream && git pull upstream master
git checkout master && git push origin master
```

if there are conflicts in the master branch you can use instead (use with caution, 
previous commits on the master branch will be erased)

```
git checkout master && git push --force origin master 
```

Note that this step will be needed everytime you want to add another contribution, if 
your forked master is not in sync with the upstream (Tencent) master (not in sync = 
is a few commits behind the Tencent master)

### 4) Create a new branch to edit it

The master branch should never be modified and always kept up to date with Tencent master

To keep your master branch clean, your changes need to be made on a new branch 
which is a copy of the master branch

To create a copy of the master branch with a different name, do : 

```
git checkout -b yourbranchname && git branch
```

### 5) Edit your branch locally

Edit your branch locally as many times as you want (you can add files, remove 
files, edit and reedit, test your code, etc..)

Test your code if you added code, (you can make a backup of the existing one, and you 
can also make backups of your code improvements every while)

When your code is finalized and has been tested, it is time to commit it !

### 6) Commit your changes and push the commit(s) to your forked yourbranchname

The code below will now : 

- configure your github username and email (identity)
- detect all changes and all them on the "to be committed list"
- commit all the "to be commited" list, in one commit locally, write a commit 
name and save and exit (on ubuntu with nano you can save and exit with ctrl+x to exit then "y" to confirm)
- display which branch we are in again, to make sure we didnt commit in master carelessly

```
git config user.name "yourgithubusername" && git config user.email "youremail@mail.com" && git add . && git commit -a && git branch
```

Now, time to push your local commit(s) to the github origin server

### 7) Push your local changes on the github origin server

Do : 

```
git push origin yourbranchname
```

it will ask your github username and password for security reasons, provide
 them then press ENTER.

Your branch is now visible on the github website

### 7B) If you want to cancel some of your latest commits/changes

If after testing/looking/discussing you want to go back to older commits, 
it is possible to remove one or many commits in your branch by doing this , 
while in your git branch 

```
git reset --hard <sha1_of_previous_commit>
```

You can find the sha1 commit in the commits list, for example here for 
Tencent/PhoenixGo master branch : 

[example of commits list](https://github.com/Tencent/PhoenixGo/commits/master)

[example of commit sha1 db3078821cd7754a98f341466d4707a985720f2d](https://github.com/Tencent/PhoenixGo/commit/db3078821cd7754a98f341466d4707a985720f2d)

so in that example, to roll back to this commit, you would need to do for example :

```
git reset --hard db3078821cd7754a98f341466d4707a985720f2d
```

note : the "hard" reset will erase forever all commits later than this commit, 
use with caution !!

Then to export your local changes to the github server website page, 
force push your local changes : 

```
git branch
git push --force origin yourbranchname 
```

### 8) Create a pull request comparing yourgithubusername yourbranchname with PhoenixGo master

On the github website, click on "create pull request", and compare your branch
 to Tencent master branch

Write some title and description, then create the topic

Your changes will be reviewed by project maintainers, and hopefully if it's all
 good it will be merged in the master branch

Pull requests can be seen [here](https://github.com/Tencent/PhoenixGo/pulls)

This is an example of pull request, for example : 
[example of pull request](https://github.com/Tencent/PhoenixGo/pull/81)

And thats it !

Hope this helps !

