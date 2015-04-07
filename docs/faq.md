Frequently Asked Questions {#faq_page}
==========================

### How do I cite CX?

There are a couple of details you will need to modify in this citation, depending on which version of CX you are using.

Hardman, K.O. (2015). CX (Version 0.1.1) [Computer Software]. Available from https://github.com/hardmanko/ofxCX/releases/tag/v0.1.1

Make sure to update the version number to whatever version you are using and get the "Retrieved from" URL by going to the release page on GitHub for the release that you are using. I choose to ignore the trailing period on the URL that APA format dictates, but you can do whatever you want about that.

If you are using a development version of the software (e.g. you downloaded the latest version of the master branch), give the version as the hash of the commit you downloaded. If you don't know which commit you downloaded, you have almost no recourse other than trying to figure out which commit you downloaded, which is hard. I strongly recommend unly using tagged releases.

IF YOU HAVE MODIFIED THE SOURCE CODE FOR CX IN ANY WAY, YOU MUST MENTION THIS IN TEXT. In this case, you may cite CX, but say that you have forked it.

### Can I have multiple versions of CX installed at the same time?

Yes, you just need to have the different versions in different directories in the openFrameworks addons directory. For example, if you wanted to have CX 0.1.1 and CX 0.1.0 both installed, you would need to have two folders in addons, one called (for example) ofxCX-0.1.1 and the other called ofxCX-0.1.0. To choose which version of CX you want to use, when you create a project with the openFrameworks project generator, you just need to select the version of the CX addon that you want to use.

### Where should I put files related to an experiment?

All files related to an experiment should go into `PROJECT_DIRECTORY/bin/data`. This makes it possible to just copy the `bin` directory when deploying an experiment, because all neccessary files are in the `data` subdirectory.

### Where are the files I created if I didn't give an absolute path?

Any files that are created (e.g. printing a CX_DataFrame to a file) for which you don't provide an absolute path go into `PROJECT_DIRECTORY/bin/data`.

### My experiment does not work on Windows XP!

See the \ref experimentDeployment page for a solution.