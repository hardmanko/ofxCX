Frequently Asked Questions {#faq_page}
==========================

### Can I have multiple versions of CX installed at the same time?

Yes, you just need to have the different versions in different directories in the openFrameworks addons directory. For example, if you wanted to have CX 0.1.1 and CX 0.1.0 both installed, you would need to have two folders in addons, one called (for example) ofxCX-0.1.1 and the other called ofxCX-0.1.0. To choose which version of CX you want to use, when you create a project with the openFrameworks project generator, you just need to select the version of the CX addon that you want to use.

### Where should I put files related to an experiment?

All files related to an experiment should go into `PROJECT_DIRECTORY/bin/data`. This makes it possible to just copy the `bin` directory when deploying an experiment, because all neccessary files are in the `data` subdirectory.

### Where are the files I created if I didn't give an absolute path?

Any files that are created (e.g. printing a CX_DataFrame to a file) for which you don't provide an absolute path go into `PROJECT_DIRECTORY/bin/data`.

### My experiment does not work on Windows XP!

See the \ref experimentDeployment page for a solution.