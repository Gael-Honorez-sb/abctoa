"""
This module implements the function for performing an indivudal test of a Maya batch render.

It must be in a separate module in order to work properly with the python multiprocessing module
"""
 
import time
import os
import shutil
import string
import system
import colorama
from colorama import Fore, Style

from build_tools import process_return_code, saferemove

colorama.init(autoreset=True)

# for windows we use a fake lock to keep the code cleaner
class DummyLock(object):
    def acquire(self):
        pass
    def release(self):
        pass

def do_run_test(args):
    return run_test(*args)

def print_banner(test_name, color_cmds=False):
    f = open('README', 'r')
    summary = f.readline().strip('\n')
    f.close()
    if color_cmds:
        print Fore.MAGENTA + Style.BRIGHT + '='*80
        print Fore.MAGENTA + Style.BRIGHT +  test_name.ljust(15) + Style.RESET_ALL + Fore.MAGENTA + summary
        print Fore.MAGENTA + Style.BRIGHT + '-'*80
    else:
        print '='*80
        print test_name.ljust(15) + summary
        print '-'*80

def run_test(test_name, lock, test_dir, cmd, output_image, reference_image, expected_result, update_reference=False, show_test_output=True, color_cmds=False):
    os.chdir(test_dir)
    
    fore_magenta = ''
    fore_cyan = ''
    fore_green = ''
    fore_red = ''
    style_bright = ''
    if color_cmds:
        fore_magenta = Fore.MAGENTA
        fore_cyan = Fore.CYAN
        fore_green = Fore.GREEN
        fore_red = Fore.RED
        style_bright = Style.BRIGHT
#	## remove any leftovers
    saferemove(output_image)
    saferemove('new.jpg')
    saferemove('ref.jpg')
    saferemove('dif.jpg')
    saferemove('STATUS')

    output_image = os.path.abspath(output_image)

    ## TODO: attach valgrind to each command
    if update_reference:
        show_test_output = False
    output_to_file = not show_test_output

    if show_test_output:
        # if we are showing output we need to lock around the entire thing, which negates all the benefits of multi-threading
        lock.acquire()
        print_banner(test_name, color_cmds)
    else:
        # otherwise, we print a little something so that we know when the process is hung, and print the rest at the end
        lock.acquire()
        print '...starting %s...' % test_name
        lock.release()
    
    output_image_dir, output_image_name = os.path.split(output_image)
    # replace test render dir with this test dir
    # image filename "testrender" and format "Tiff" are set in the test scene render settings
    # cmd = string.replace(cmd, "test.log", os.path.join(os.path.abspath(os.path.dirname(test_env['OUTPUT_IMAGE'])), (test_env['test_name']+'.log')))
    cmd = string.replace(cmd, "%proj%", output_image_dir)
    cmd = string.replace(cmd, "%dir%", output_image_dir)
    cmd = string.replace(cmd, "%file%", os.path.join(output_image_dir, 'test.ma'))
    
    # verbose and log options for Render cmd : -verb -log "test.log" 
    if show_test_output:
        cmd = string.replace(cmd, "%options%", '-im %s' % os.path.splitext(output_image_name)[0])
        print fore_cyan + cmd
        print fore_magenta + style_bright + '-'*80
    else:
        logfile = "%s.log" % (test_name)
        logfile = os.path.join(output_image_dir, logfile)
        cmd = string.replace(cmd, "%options%", '-log %s -im %s' % (logfile, os.path.splitext(output_image_name)[0]))
        if system.os() == 'windows':
            cmd = '%s  1> "%s" 2>&1' % (cmd, logfile)
        else:
            cmd = '%s  > "%s" 2>> "%s"' % (cmd, logfile, logfile)
    
    before_time = time.time()
    try:
        retcode = os.system(cmd)
    except KeyboardInterrupt:
        print "child keyboard interrupt"
        return

    running_time = time.time() - before_time
                          
    ## redirect test output (if not using os.system
    # if output_to_file:
        # file = open("%s.log" % (test_name), 'w')
        # p = subprocess.Popen(cmd, stdout = file, stderr = subprocess.STDOUT)
        # retcode = p.wait()
        # file.close()
    # else:
        # p = subprocess.Popen(cmd)
        # retcode = p.wait()

    status = process_return_code(retcode)
    if expected_result == 'FAILED':
        # In this case, retcode interpretation is fliped (except for crashed tests)
        if status == 'FAILED':
            status = 'OK'
        elif status == 'OK':
            status = 'FAILED'
    elif expected_result == 'CRASHED':
        if status == 'CRASHED':
            status = 'OK'
        elif status == 'OK':
            status = 'FAILED'
    if status != 'OK':
        cause = 'non-zero return code'

    has_diff = False
    cause = None
    if status =='OK' and expected_result == 'OK' and reference_image != '':
        if update_reference:
            ## the user wants to update the reference image and log
            ## NOTE(boulos): For some reason reference.tif might be read
            ## only, so just remove it first.
            saferemove(reference_image)
            shutil.copy(output_image, reference_image)
            reference_log = os.path.join(os.path.dirname(reference_image), 'reference.log')
            
            print 'Updating %s ...' % (reference_image)
            print 'Updating %s ...' % (reference_log)
            shutil.copy('%s.log' % test_name, reference_log)

        if os.path.exists(output_image) and os.path.exists(reference_image):
            ## if the test passed - compare the generated image to the reference
            difftiff_cmd = 'difftiff -f compare.tif -a 0.5 -m 27 %s %s' % (output_image, reference_image)
            if show_test_output:
                print difftiff_cmd
            else:
                if system.os() == 'windows':
                    difftiff_cmd = '%s 1> %s.diff.log 2>&1' % (difftiff_cmd, test_name)
                else:
                    difftiff_cmd = '%s > %s.diff.log 2>>%s.diff.log' % (difftiff_cmd, test_name, test_name)
            diff_retcode = os.system(difftiff_cmd)
            if diff_retcode != 0:
                status = 'FAILED'
                cause = 'images differ'
                ## run difftiff again (!) to make one with a solid alpha...
                if system.os() == 'windows':
                    os.system('difftiff -s -f dif.tif %s %s > nul 2> nul' % (output_image, reference_image))
                else:
                    os.system('difftiff -s -f dif.tif %s %s > /dev/null 2>> /dev/null' % (output_image, reference_image))
                os.system('tiff2jpeg dif.tif dif.jpg')
                saferemove('dif.tif')

        ## convert these to jpg form for makeweb
        if os.path.exists(output_image):
            os.system('tiff2jpeg %s new.jpg' % (output_image))
        else:
            status = 'FAILED'
            cause = "output image does not exist: '%s'" % output_image

    if expected_result == 'OK' and reference_image != '' and os.path.exists(reference_image):
        os.system('tiff2jpeg %s ref.jpg' % (reference_image))

    if show_test_output:
        print fore_magenta + style_bright + '-'*80
    else:
        lock.acquire()
        print_banner(test_name, color_cmds)
        print "logged to", logfile

    print '%s %s' % ('time'.ljust(15), running_time)
        
    ## progress text (scream if the test didn't pass)
    
    if status == 'OK':
        print fore_green + '%s %s' % ('status'.ljust(15), status)
    else:
        print fore_red + '%s %s' % ('status'.ljust(15), status)
        print fore_red + '%s %s' % ('cause'.ljust(15), cause)
        

    lock.release()

    ## get README so that we can stick it inside the html file
    f = open('README', 'r')
    readme = ''
    for line in f:
        readme += line
    f.close()

    ## create the html file with the results
    f = open('STATUS', 'w')
    f.write(status) ## so we can get the status of this test later
    f.close()
    html_file = test_name + '.html'
    saferemove(html_file)
    f = open(html_file, 'w')
    f.write('''
<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">
<html xmlns="http://www.w3.org/1999/xhtml" lang="en" xml:lang="en">
<head>
<title>
Arnold testsuite - %s
</title>
</head>
<body>
<table border="0" cellpadding="0">
<tr>
<th><font face="Arial">test</font></th>
<th><font face="Arial">status</font></th>
<th><font face="Arial">description</font></th>
<th><font face="Arial">new</font></th>
<th><font face="Arial">ref</font></th>
<th><font face="Arial">diff</font></th>
</tr>
<tr>
<td bgcolor="#ececec">
<center>
<font face="Arial">
&nbsp;%s&nbsp;
</font>
</center>
</td>
<td bgcolor="#ececec">
<center>
<font face="Arial">
&nbsp;%s&nbsp;
</font>
</center>
</td>
<td bgcolor="#ececec">
&nbsp;
<pre>
%s
</pre>
&nbsp;
</td>
<td bgcolor="#ececec">
<font face="Arial">
  %s
</font>
</td>
<td bgcolor="#ececec">
<font face="Arial">
  %s
</font>
</td>
<td bgcolor="#ececec">
<font face="Arial">
  %s
</font>
</td>
</tr>
</table>
<font face="Arial">
<a href=".">link to files</a>
</font>
</body>
</html>''' % (test_name,
                  test_name,
                  status,
                  readme,
                  os.path.exists('new.jpg') and '''<img src="new.jpg" border="0" hspace="1" width="160" height="120" alt="new image" />'''
                                                      or '''&nbsp;''',
                  os.path.exists('ref.jpg') and '''<img src="ref.jpg" border="0" hspace="1" width="160" height="120" alt="ref image" />'''
                                                      or '''&nbsp;''',
                  os.path.exists('dif.jpg') and '''<img src="dif.jpg" border="0" hspace="1" width="160" height="120" alt="difference image" />'''
                                                      or '''&nbsp;<b>no difference</b>&nbsp;'''
                  ))
    f.close()

    return running_time
