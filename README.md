This repo is for shim audit requests for signing.

Here's how to do it:

- Clone it
- Run: ./make-my-branch -r $repourl -t $tagname $ESPNAME-$product-$version-$YYYYMMDD
  Where:
  |variable|meaning|
  |------|------|
  | $repourl | the url of your git repo
  | $tagname | the name of the tag or branch that your patches are on
  | $ESPNAME | who you are in a very basic form.  Basically your EFI System Partition directory reservation in lower case.  For Red Hat, it's "redhat", etc.  For Debain, it's "debian", etc.
  | $product | simple name of the product this is for; for Red Hat Enterprise Linux, it's "rhel"; for debian it might be "jessie".  You get the idea.
  | $version | which release this is for, for Red Hat Enterprise Linux it's "7.4", for example.
  | $YYYYMMDD | the current date when you're doing this.
  |-----|------|
  

- Copy your .efi files to assets/
- edit assets/readme.txt and assets/method.txt to say the appropriate things.
- git add assets/\*
- git commit --sign -a
- send a pull request for your branch on the github repo at https://github.com/vathpela/shim-review .
