/*
mobblerbrowsercontrol.cpp

Mobbler, a Last.fm mobile scrobbler for Symbian smartphones.
Copyright (C) 2009  Michael Coffey

http://code.google.com/p/mobbler

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#include "mobblerbrowsercontrol.h"

#include "mobbler.rsg.h"
#include "mobbler_strings.rsg.h"
#include "mobblerappui.h"
#include "mobblerliterals.h"
#include "mobblerlogging.h"
#include "mobblerparser.h"
#include "mobblerresourcereader.h"
#include "mobblerstring.h"
#include "mobblertrack.h"

#include <aknnotewrappers.h>
#include <brctlinterface.h>
#include <charconv.h>

/*_LIT8(KArtistInfoHtmlTemplate,"<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.0 Transitional//EN\">\
<html><head><title>%S</title></head>\
<body style=\"font-family: arial, sans-serif;\">\
<h3>%S</h3>\
<p>\
%S\
</p>\
</body>\
</html>");*/
// background: url('background.gif') no-repeat fixed 5%% 20%%; <-- goes in style tag

//.even { background: #fff url(data:image/gif;base64,R0lGODlhBgASALMAAOfn5+rq6uvr6+zs7O7u7vHx8fPz8/b29vj4+P39/f///wAAAAAAAAAAAAAAAAAAACwAAAAABgASAAAIMAAVCBxIsKDBgwgTDkzAsKGAhxARSJx4oKJFAxgzFtjIkYDHjwNCigxAsiSAkygDAgA7) repeat-x bottom}
//background-image:url(data:image/jpeg;base64,/9j/4AAQSkZJRgABAQAAAQABAAD//gA7Q1JFQVRPUjogZ2QtanBlZyB2MS4wICh1c2luZyBJSkcgSlBFRyB2NjIpLCBxdWFsaXR5ID0gOTAK/9sAQwADAgIDAgIDAwMDBAMDBAUIBQUEBAUKBwcGCAwKDAwLCgsLDQ4SEA0OEQ4LCxAWEBETFBUVFQwPFxgWFBgSFBUU/9sAQwEDBAQFBAUJBQUJFA0LDRQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQU/8AAEQgAlgCWAwEiAAIRAQMRAf/EAB8AAAEFAQEBAQEBAAAAAAAAAAABAgMEBQYHCAkKC//EALUQAAIBAwMCBAMFBQQEAAABfQECAwAEEQUSITFBBhNRYQcicRQygZGhCCNCscEVUtHwJDNicoIJChYXGBkaJSYnKCkqNDU2Nzg5OkNERUZHSElKU1RVVldYWVpjZGVmZ2hpanN0dXZ3eHl6g4SFhoeIiYqSk5SVlpeYmZqio6Slpqeoqaqys7S1tre4ubrCw8TFxsfIycrS09TV1tfY2drh4uPk5ebn6Onq8fLz9PX29/j5+v/EAB8BAAMBAQEBAQEBAQEAAAAAAAABAgMEBQYHCAkKC//EALURAAIBAgQEAwQHBQQEAAECdwABAgMRBAUhMQYSQVEHYXETIjKBCBRCkaGxwQkjM1LwFWJy0QoWJDThJfEXGBkaJicoKSo1Njc4OTpDREVGR0hJSlNUVVZXWFlaY2RlZmdoaWpzdHV2d3h5eoKDhIWGh4iJipKTlJWWl5iZmqKjpKWmp6ipqrKztLW2t7i5usLDxMXGx8jJytLT1NXW19jZ2uLj5OXm5+jp6vLz9PX29/j5+v/aAAwDAQACEQMRAD8A/POg8elAoPTnpXWchG7d88V2vwf+C/jH48eKBofhDSnvpRg3F052QWq/3pH6D6ck9hXNeE/C2oeP/F+j+GdGj83U9UuktYFIJAZiBk47Dkn2FfvP+zd+z74c/Z9+G+n6FpUMeYkEl3elAJbqbA3SOfU9h2AArJys9Ckras+OPh1/wSI0owRT+MvF2oXlwQN1vpCJbxhu43OHJH4CvTj/AMEofhEEwBrhP/YR/wDsa+wJ9blZ9sP7qPsccmov7Vuv+ex/St1SqS1OJ4+ina1z89/iN/wSO0tIJpvBvi6+tZ1X5LfV40nRj7sgUgfga+Cfi78FfF3wP8Ttovi3S3sZmJMFwvzQXC/3o3HB6jjqO4Ff0CW2syKwE37xfpzXm37SfwC8P/H74bX+h6lDGHkRpLW8VQXtpwPlkX6HGR3HFZSTg7M6KdWniL8m6PwNBB6UVr+MPCt94G8Wax4e1NBHqGmXT2k6j+8hxn6HqPYisiq2LD8qKKKYBR+VFFABRRRQAflRRRQAflRRRTAO1Md8AgU+on6mgD6T/wCCaXhOHxZ+1dp884Vho+nXOoKrf3spCD+HnZr9sdZYxWNvEpIB5P4V+OP/AASeAP7UWrZ5/wCKbuP/AEptq/YrxE22K25x1/pWcP4qMcW7YeTRlcnrzRiofN96PNH9416p8pzExFbGkZmsLmM8hc49sisHzR6mtzw+xa3u8HsP5GubEL92zvwEmsRFLqfi9/wUe8K2/hz9qPWprcKg1S0t710UYAfZ5Z/Py8/jXzJX1x/wVDyv7TShuD/ZFv1/35K+RiQOprkh8KPo3uLRTd49aXePWqELRSBgehpSQByaYBRRRSAKKKKACiiimAVFJ1NS1E/U0gPrb/gk7/ydHq3/AGLdx/6U21fsP4nOIbXHv/Svx4/4JO/8nR6t/wBi3cf+lNtX7CeK22x2nvu/pTp/xUc2OdsLL+upheYaTeaZ5ntR5ntXqcp8Zzkm810fhX54rgHvgVzHme1TW2pXFnv8mQx7uuMVjVpOceVHXhcQqNZTktEeU/Hr9hH4fftBeNl8UeJBqf8Aaa2yWo+x3XloUUkjjaefmNean/gk98JGH3te/wDBh/8AY19S/wBuX/8Az8N+lH9uX/8Az8N+lciw1RaXPa/tej/Kz5Z/4dOfCP8Ava9/4MP/ALGj/h058I/72vf+DD/7Gvqb+3b7/n4aj+3b7/n4aj6vU7h/a9D+Vny0P+CT3wjH8Wvf+DD/AOxpJP8Agk/8JShCya8p9RqHT81r6m/t2+/5+Go/t2/Az9qb6cUfV6ncazag94s+BviV/wAEjbRbae48GeLby2mUbktdWhEyN7GRNpX67TXwT8VvhH4s+C3id9C8WaVJp14Pmik+9FOmcbo36MOO3Tviv38sPFUqPtuyssZ745H+NeaftUfs6eH/ANoP4ZXumXcES3wjaXTtQVRvt5gMqQfQ9CO4NYTjOk/ePRw9elik3Tep+Do5FLVzXdGvPDmu6jpGowm3v7C5ktbiJuqyIxVh+YNU6s2Cij/PSigQVFJ1NS1E/U0AfW3/AASd/wCTo9W/7Fu4/wDSm2r9gPGLARWfP97+lfkB/wAEnf8Ak6PVv+xbuP8A0ptq/Xrxq4WGyz/tf0p0FesjkzB2wcm/61Oe3+9G73qmZjnjGKTzG9a97lPgvaIueb/tUGUD+KqOT6mgEg9aOUj2jLv2gDvmka5yOKqbjRvPtRysPaMs/aGo+0N2NVt5o3mjlYc9yz55PcikMgPU1X3mjeaOVhzE5denauu8MSG70OeFuRG5UZ9CBXFFzXa+EozDotxKekj5Ge/Arhxa/dntZPKTxVltZn4n/t8+GIPCv7U/i2K3I8u98m+ZQMYaSMFv1BP418+19D/t/wDiK38SftU+K3tvmSzSCzZgcgukY3fqcfhXzzXmrY+ye4lFLRTEJUT9TUtRP1NAH1v/AMEnf+To9W/7Fu4/9Kbav118eHEVj/wL+lfkV/wSd/5Oj1b/ALFu4/8ASm2r9c/HxAisM/7X8hV4f+OjhzR2wMv66nJ76N/tUe8etG9fWvoLM/OeYk3+1T2tnc3ufIt5JQvUopOKqb19a7H4en93f+m5D+hrnrzdKm5o78DQWLrxpSdkzB/sXUP+fKf/AL9mj+xNQ/58p/8Av2a5P4y/ts/Cr4EeLB4b8YeI20rVzAtytuLC5nzGxIDbo42Xqp4zniuC/wCHofwE/wChyb/wU3v/AMZrzPr1T+VH1H9hUf8An4/wPaf7E1D/AJ8p/wDv2aP7E1D/AJ8p/wDv2a8W/wCHofwE/wChyb/wU3v/AMZo/wCHofwE/wChyb/wU3v/AMZp/Xqn8qH/AGFR/wCfj/A9p/sTUP8Anyn/AO/Zo/sXUP8Anyn/AO/Zrxb/AIeh/AT/AKHJv/BTe/8Axmkf/gqF8Bdp2+MZGbHAXSbzJ/8AIVH16p/Khf2FR/5+P8D3rTfCN5dSg3ANrCPvZxuP0Fcz+0l8dNA/Z7+Feo65qEiAQR+VZ2qNh7iY8JGv1PJPYAmvlH4mf8FdvB1jbTweDtC1TXrzGI5rpBawZ98kvj/gNfnr8Zvjv4y/aB8UHW/Fuo+cUyttYwApbWqnsi5P4kkk+vauepVlWacj1MNhKOCTVPVvqcr4m8SXvjHxPq2vak/mX+qXUt5O3+3IxY49snis+kAA9KXNI6Q/z0ooooAO1Rv1NSdqjfvQB9a/8EnT/wAZRat/2Ldx/wClNtX64/EQ7YdPx/t/0r8df+CXupfYP2srePzFQXWkXcJUnG/BjfA/74z+FfsX8RObCwk64Yj8x/8AWq8N/vEUcGa/7jP5fmcVvNJvNR+b7UeZ7V9Pyn5jzEm812/w9ObXUD3yv8jXCeZ7V1vw7vFW+ubZj/rkDKPcZz/P9K4sbBuhKx7OT1FDGwcvP8j8m/8AgqZbrN+06hIzjRrcf+PyV8hfYE/u1+kP/BWL4JaodY0H4j6dZy3VjFB/Z2otEufJwxeKRv8AZOWXP+7X5y+cpIHevnI6n6NLcg+wJ/do+wJ/dqczKDSeetXYiz7kP2BP7tH2FAegqbz1pRMpOKOUNRscCjkjmpgMDAFN3jtzTqpDCij/AD0opgFFH+elFAB2prgbTTu1I/KmgD1z9h/xKnhT9rPwBcu4jjubqSyc4H/LWJ1Xr/tFa/c7xxH5/haGYDJjkU8e/H9a/nO0bXp/B3irRvEFmSLrTLyG8j5xyjhh/Kv6LPD2rW3j34cwXli4mtb+0S4t5P7ysoZT+opU37OrGfmZYun7bDTprszzzcc/jS5PrURYhiCMEcEUeYa+v5bbH47zEuT61PYX82nXsVzC2JI23D39vxqn5ho3mny3VnsONVwkpReqPVLbX9H8T2fl3TRxysMPDNjB/PgiuF1X9lP4S+IJHlvPA3hy5d8FmbT4ucdOgrELZxkA47U+Od4/usV/3eP5V488ri3eErH19LiWcY2q07sk/wCGKfgwevw+8N5/68I/8KP+GKfgx/0T7w5/4AR/4Un2ub/nq/8A30aT7VN/z1f/AL6NR/Zkv5zb/WeP/Pn8f+AO/wCGKfgx/wBE+8Of+AEf+FMl/Yn+DDxsv/CvfDuD/dsEH9KX7VN/z1f/AL6NL9rmx/rX+u40f2ZL+ca4nh1o/j/wDyz4mf8ABMb4S+MLWd9I0248MXzjKTaVOVRT7xvuXHsAPqK/Nf8AaR/ZV8Xfs2a6sOrp/aGh3DFbTWIFxHJ/sOP4HHoevYnt+x+j+MNQ0eZf3rXEGfmilYnj2J6VpfGL4Z6B8dPhhqWi6lAlxp+oQFd20FonHKuPRlYD8q4a+HqYZrmd0z38BmVDMYtQVpLofz70Vt+O/CN78P8AxvrvhnUP+PvSb2WzdgMB9jEBh7EAEexrErA9IP8APWiiimIO1BOBR2ooAzdRt/Njr9iv+CXPx3i+InwJtfDV5cBta8MN/Z8sZPzNDjML/Tb8n/bOvyDlRWXH6V6N+y38d7v9m/4z6Z4jWSQ6JORa6tAgzvt2/ix/eQ4YfTHespptGkep+1/jrSzouuOyri3uMyxn3J+YVzv2uu40vX9H+K3gq0vbC8ju7S7iS4tLuI5UhlyrD2II96821O3udGu5La7QxSqfwYeoNfVYDERrw5JfEvxPynOsHLCV3Uivclr6PsaP2uj7XWJ9uz7Un26vT5UfOe1Nz7XR9rrD+30v26nyIPam39ro+11ifbqPt1HIg9qbf2uj7XWJ9uz7UfbsUciD2pt/a/evTPhteGfw/fRMcqkvyj0yB/n8a8Y+3V6Z8M7tovDl9K2Qrz8Me+FANeXmUf8AZ36o+i4fq/7en5M/In/goPpdrpf7VnigWqBBcRW08gHQu0S5P6V8619B/t76pDq/7UviqSKQSCKO2hJHZhCuR+FfPlfMo/U731D/AD0oo/z1opgHaijtRQAjKGPNUr22EwbA5xV6kZQ3WhoZ9Bfsf/tr6t+ztdR+HdfWXU/BMshYBMtNYk9WT1Q919sj0r9V/C3xH8IfGTwxbalpeoWusafcDMdzbSBip9MjlSO6n8RX4OXFqkmeOa2/h/8AE3xj8ItV/tDwlrt1pEpPzxI26KQejo2Vb8qSk6bvFnNWw8MRHkkr37n7a6p4AuoSZNPnS8j67GbY4/of0rmrvTNTsifOsbiMD+Ixkj8xXxN8PP8AgqJremrHB4x8MLeAYDXekybGI9fLc4z/AMCr3zw1/wAFIfhZrSKbnVLzR3bql/ZuCDx3TcO/r2r2qeZ1IfHqfFYrhum2+S8fTVHqDzyRffR1/wB4EU37b9ay9P8A20/hPqkhij8a6QGC7j5s4jGM4/iwO9aX/DWnwt/6HTQP/A+L/GulZomvhPI/1dqL7b+4d9t+tH23603/AIa0+Fn/AEOmgf8AgfF/jSf8NafCz/oddA/8D4v8af8Aai/l/EP9Xqn/AD8/Bj/tv1pDfcd6T/hrT4Wf9DpoH/gfF/jSH9rP4WkceNPD5P8A1/Rf40f2oukfxBcPVH9t/czZ0TQ9R1ydVhiMcORumk4VR/U+1dL8UfijoXwS+Gt/qV/crFZafAW5IDyvnhQO7MxA/Gvnr4gf8FEPhp4Vhnh07UpvEF4owsOmQllLenmEhce4Jr4F+O/7Rvin9oDXBPq0v2LRoGJtdKhbMaf7TH+Jsdz07YrzMVjJYj3Xol0PqspyhYG8t292/wBDi/G3i+7+IHjTW/El+MXWqXcl065zt3tkKD6AYH4VjUi9BS1559UtAoo/z0ooAO1FFFABRRRQAmBmmSIM9BRRQBHJbDFRNYoVzxRRQMjGnoOwpP7Pj9B+VFFKyAP7Pj9B+VH9nx+g/KiiiyC7D+z4/QflTl0+PcOB+VFFFkBYjs0QHFSoKKKAux+KKKKYgooooA//2Q%3D%3D);

/*
 * Format string specifiers are:
 *
 * %S - artist name
 * %S - image url
 * %d - image width
 * %S - tags
 * %S - similar artists
 * %S - bio
 */

_LIT8(KArtistInfoHtmlTemplate, "<?xml version=\"1.0\" encoding=\"iso-8859-1\"?>\
	<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Strict//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd\">\
	<html xmlns=\"http://www.w3.org/1999/xhtml\">\
\
	<head>\
		<meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\" />\
		<title></title>\
		<style type=\"text/css\">\
			body, html {\
				margin: 0;\
				padding: 0px;\
				font-family: Arial, Helvetica, sans-serif;\
				font-size: medium;\
				color: #333333;\
				background: url(data:image/gif;base64,\
R0lGODlhpwCnAPcAAPzq7Pz29Pzy7Pz+/Pz6/Pzu7Pz69Pzy9B5zAR/kAACQCgB8AABsjADlEBYS\
EwAAAAoYBgAAAAAAAAAAAAAG+AIA2wAK/QAAfwDcrAME5gAiEgAAANi2F+QSsxJzQgB0funRBuUY\
AIFDCnx+AAAD3AAABAEAIgAAAFYAAAAAAAAAAAAAAOCtJuPhsxJDQgB+fnOKAQDhAABDCgB+AAAG\
AOUA0BIA/QAAfyDcAekEAJAiAHwAAGAAiAAA5pEAEnwAAP8BCP8A5/8AEv8AAF0GmAAA55EAEnwA\
AIUGj+cABIEKRHwAfgDcMAAEsxYiQgAAfmAA/wMA/wAA/wAA/3hOJnEWsx/2QgAAfjghOGADAhYk\
QwAAfgAG4AAA5gAAEgAAAH7cAAAEAAAiAMAAAAAASAAAAgAAQwAAfv8GAP8AAP8AAP8AAP8GAP8A\
AP8KAP8AAABwBADmAAASAAAAAAAA8AAA5gAAEgAAAADIHADj5xYBEgAAAF4Fc1oA5AAAkAAAfBA0\
4OXl5hISEgAAAJ8gAOvpAIGQAHx8AEsoB+PmAIESAHwAAGAgANrpAFCQAAB8ADhgAGAAAAGRFgB8\
AGwAAAAAAAAAAAAAAExeAORaABIAAAAAADQoAADmAAASAMAAAKC1APkrABKDAAB8ACAASOkAAJAA\
7HwAAGAAAAAAAJEAAHwAAP8AAP8AAP8AAP8AAF0AMgABAJEAAHwAAOoZMvQsAICDAHx8AABMHADm\
7BYSEgAAAAD0/wAr/wCD/wB8/zgAAGAAABYAAAAAAAAYVAEA5wAAEgAAAABkBgBknQCDTAB8APcA\
NPQA54AAEnwAADhQd+bncBISTwAAADidaGBk5xaDEgB8ADBEZLONZEJPg34AfP9gGv+Y6f9PEv8A\
ACYAzLMB/0IA/34AfzhM6ALm6EMSEn4AAGwAAOUBABIAAAAAAAC4ZABkZACDgwB8fEgBAAIAAEMA\
AH4AAAAAXgAAWgAAAAAAAAABTAAAAAAK7AAAAAQAgQDQRQD9SAB/ACH5BAEAAAMALAAAAACnAKcA\
Rwj/AAcIHEiwoMGDCBMqXMiwocOHECNKnEixokADGDEC2Mixo8ePHzOKtEiypMmGIDkWKHAAwAGW\
LlmunAlgps2WOG+6rGlzZcuVO1/a5Jmyo4GTSCkSANkT6EycQWHSnNq0QNCdQF8KrZkza8ycP3tC\
DUsz5dGkJz96bUrWKtirQ6k2jQoV5kuuNbFqtcv3Z0ynVWl2/RgA7cOOU7/y9IlVMF3FVckSfSqV\
69SwPMdK3ct47k3HeyUbNVzQ41CtjXXmXPwzAAGFBAi4PkD7QIDbsV8nJID6pm7SAoV27dmxcFLT\
px9vlsryd/DIziUGsPpRAHCJBKinNG4ROebeVju7/3VJMLtT7gNaFl1vlqD2mgWzs5+P2KF8kOgj\
Ih8fnnlZtQQJUN9A6tFnIABnLbTUgQYW4FCBhE2031AMbuTgQAKqRBCEFRaVIGwdznchQxx69CFK\
IYoY4IDBpbjeiAqV6GJxDC1YlH4HNjfRewBsOOOPQAb5EY7brSjkRj4eqeSS9ElY1G82BgmjAUxW\
aSWSFVVYwInSXeklk0h9KeaYQV43AJlopumRmQup6eaPbKb1pphx1mnnnXjmqeeefPbpp31Udtjf\
U4vplFiKAmD0552BqqXaY5jJpRpbkLmV3HCFEooZSIkuephpQlkaF2OaSSbppUS9RapekNpVaFuB\
5f8Vkqf7sUoocaVilampl1X6l3hkvYVasIu9OtmqfploZ5TFutpWXaDZ6tMBucE2QGy1ZZYXX5bh\
5RRtsTWEbaid/TQcj1zKaWGvqjqm6QEGyRaquTLd+mioAuT3p3lzjWaSd6xVSqyOA013k0HTCXoi\
swDoW16GFToMoofdIebTZsYGhh7E8BHIZILMQskkjDEWJTFDFgdMoYoYspjex+KO/OB66So04Y8w\
cgyjjD/WHF+VM1P8aZXwtqyhx1d+KBuPVm5JkAE8KwsRewVERwDH9GHdI9Jzdp2SREVZJxDDQO7s\
9dkcZclefmTTV3VBUaNNZkly1z03WnbnXaadevf//bWnAvmdJuAV+00y4YgnrvjijDfu+OOQR85o\
o2cn6rPkpAXgdqyBnbuqWM0mO1+nmKvNlKaKgQcY6OdC62205BZabEqkl/6zo3KNVemgmUbLOsah\
Qto7sjKFLTnlKnGO7O7NVgWXV5FydtWw/pEa2H8eiZ24WrBGmpWz9a4+/Hi9mRuTt2EJ66p4g8Yq\
Okfa+4kcXun3Pr2zza9+qX91zatpX1+xV39+9S3IZI9PEwof6lA1rOERMDwBA0/s/II66n1HLioT\
DF+wh6BlWayAqoMW9FK3PtC1Lzze+462rJc+BcLqVsHL1QD9dZ2beW6BoBGMa07CGxMCi1o85E2x\
/zAFFBoe54OW0oyxzPeU6IyNNsspFwxZssN4+eRkwIEiubCHRSetC3Yb5M/B4PZCllDLiQPJzQEE\
0BYLXS5PW+RRF6eWvP35ZV6scQ6/tALEg9ymiPALQM00N6uH/FEt+UKjuJjGkTmiSCXkUpVlNIUe\
g80kOoysUPwGgC4/7i1o+CEJwAQ4MJkUTSCETFvBwLRKI2bSRZtMSNTeaLMPqjCOgNxI/HSWJCUp\
zUJ6BFpDZkmRWpXxlRYyUjK5diRaXkuYJKIZ2JBIHAblzGVxS5Ez2wanYUqTSF/MpZaU2bEWLWmb\
0FxI3JxJENMI6ZpHM6eSHMnNGTlEa0Z85JJ2if9NKy0EmT+KZXnWBk4t0cY1uclNFQXCo14KTk3s\
HIiBakYAA9xmjbcxACbXxMyHkimigVtPfgDKIId6VEwgDenf5Hkkk570SsWknUsD6p6XfqlwK2Wp\
i9BTT5sCiW700de4UmJGNKbSp6wEaooE2ZAA4BOpS8IbVKfKUcNQFap1uqpH9aTVutGqq4NjHFjT\
KbmxVsh2DgErWklzJQgebq1wjatc50rXutr1rnjNq173WleR+PWvGBGkRQerKL4uSiRPtZIgmWpY\
0hggsWgrbGMhghHIIiYvDXSd7k7jH6IcSKOTRYhl9Xc/3bVRhPizngZJmpHGPrYopKWK8FD7PG7/\
3cVzrUImaPGaWNSS1oLTk50IA3g/O0ZldiBRJOaeiscXRhCAmhmhqX4VFVzhD1ayGhJa8Qm9Eo5K\
faZ94AZX2N3fBncuWwmJchGnteU10DPQve4Dz0Xc742qP5uV7l+S2zjkpYp8qbMfCwMcXdV+TzJx\
zF8Yg6tE7SYOazmkH7eIQz7oFte+gBFvG3NnKQJf2MGeQh7rGkM9/XVXWB9e7fd+dRfeWXe2Fxbn\
1v4EYQLeFsBb7B14GZy7Ia6YVy6mLvV4nOGq6sm/N7xtjE+oRBQTcbwtViEGk1Ph1QCwXx1ZL3BE\
3GElEwu3n4luq3i1ldUIJzUutm18LWPllXFE/8to4fJg8gu+++L4fH0pYWbKG2A7Y1aDKK5whu9i\
ZDaJOICAEWGBxXevA9crhsXb4I2x2+IOX1dU1aVKet8cp0OrDod1vp5YakMt1wSA1Fts7mdEtepp\
2WY28qoNep2H6SybSc4PPC/xVAyuisiLMhecpE/CNZGhXoyA++3IdTyN7PD+zjek6eGkhp05znQX\
xEnR2XhHfNpHK/fUUDSvGRdqkEiekk3Sxuz5Cm0Syh04U2xmMWvOXTDayI7VGtwVscvTRzjy2dZI\
cbegMTicrujr1D++yUFtQ2pRC6B2i7K2OJMi8Ez/Nt/9Dk6Jew2RX4fSU/zK5EncveQu/7cA+f+x\
N37hTMv7SI0iKVUIMts9oSdD+i88tXbGL0JSXTqxRPRGyGvd5kiDzJwkAgewppXuE35Du50z2mhH\
BIrKGb21ljmdSNIr/N8o/8o5AhijRIXkHA7pK5s5MuSNLLJ1KCdxz01vJTAH4t8foec9JLNsh4YG\
koqQvMV4tjIu7z71mioJRkcFwG/qbve0rl3rFrtgefuFHo+cu0pQ0s7bqr6koCPklRU5KqtvyMGN\
XL4jp49qfPbNeSV5/iCgh3zyWKxAGaMe6hxJvZLgPIDEB+n1Rn+S7NdF+p7rnjxjVz1DfA8k4Bfk\
6AW9WIINdPzjk72pTHK+4bPekLa7mfq4N33/+FuK/c47BPoP8b6Lqj/+6y8/++dfT5fq2PP1sD/5\
uy+/6+P/ePvU6kf3p1L5937mp099R1lWEoBnwiS8p3eC4nj9p38FKIDIR4HkpxCMByRzBFAFtX/4\
V4ELWCUCoEj15yKKlIGqlH5XUnm394FNY3tfshIOmIIOgYJEtXMo04IWaFbdpILUx3sIYXntx4Mu\
Mn/250RLwx75MgDIY31EmCJGyBTRMYPs4YRPuHcdZ38DdSRWeIUHIhEZiHgTGIJeiIURkYHOAX8u\
WIZNMnwfsXhquINsyB4wVxTnVoIGAodz+IV1KDQ6FXVDuIfch4AiVRBUKIVPI4htaBHMZxpWnpMw\
boODh8iGJtGIH3cQCYVQymUAeDiH6sIgBxBzvaeIfGgSPWUgbLRYFnUbk0iKWDJyrmg3hmGJsYgm\
W1aLc8ImtIiLZHWLvEgnd7KLv2hPRzaMyrcnwmiM8wE4NqiMg/gnzeiMr7g40fiLmFONpAhX2HiF\
d7WNWuVaTxhaQehTVyeOWNc15WiOJOFW25Jd2KaO8BiP8jiP9FiPBREQADs=)\
no-repeat fixed 5%% 20%%; \
				\
			}\
\
			#nav_bar {\
				width: 100%%;\
				text-align: center;\
				background-color: #ff0000;\
				color: #ffffff;\
				padding: 1px;\
			}\
\
			#info_bar {\
				width: 100%%;\
			}\
\
			#scrobbled_tracks {\
				float: left;\
				width: 35%%;\
			}\
\
			#artist_image {\
				float:right;\
				width: auto;\
			}\
\
			#body_text {\
				width: 100%%;\
				float: left;\
			}\
\
		</style>\
	</head>\
	<body>\
		<div id=\"nav_bar\"><h3>%S</h3></div>\
		<div id=\"info_bar\">\
		<div id=\"artist_image\"><img src=\"%S\" width=\"%d\"  /></div>\
		<a name=\"#tags\"><p><strong>Tags: </strong>%S</p></a>\
		<a name=\"#similar_artists\"><p><strong>Similar Artists: </strong>%S</p></a>\
		</div>\
		<div id=\"body_text\">\
\
		<p>%S</p>\
\
		</div><!-- body_text -->\
	</body>\
</html>");

_LIT8(KHtmlDataType, "text/html");

CMobblerBrowserControl* CMobblerBrowserControl::NewL(const TRect& aRect, CMobblerAppUi& aMobblerAppUi)
	{
	CMobblerBrowserControl* self(new(ELeave) CMobblerBrowserControl(aMobblerAppUi));
	CleanupStack::PushL(self);
	self->ConstructL(aRect);
	CleanupStack::Pop(self);
	return self;
	}

CMobblerBrowserControl::CMobblerBrowserControl(CMobblerAppUi& aMobblerAppUi) :
	iAppUi(aMobblerAppUi)
	{
	}

void CMobblerBrowserControl::ConstructL(const TRect& aRect)
	{
	_LIT8(KGetInfo, "getinfo");
	
	CreateWindowL();
	SetRect(aRect);
	SetExtentToWholeScreen(); // Full screen
	//iAppUi.StatusPane()->MakeVisible(ETrue);


	if (iBrCtlInterface)
		{
		delete iBrCtlInterface;
		iBrCtlInterface = NULL;
		}

	// TODO: need to detect 3rd Ed and use
	// TRect brCtlRect(Position(), Size());   // in 3rd Ed devices
	TRect brCtlRect(TPoint(0, 0), Size()); // in 3rd Ed, FP1 devices

	iBrCtlInterface = ::CreateBrowserControlL(
			this,
			brCtlRect,
			TBrCtlDefs::ECapabilityDisplayScrollBar | TBrCtlDefs::ECapabilityLoadHttpFw,
			TBrCtlDefs::ECommandIdBase);

	iAppUi.LastFmConnection().WebServicesCallL(KArtist, KGetInfo, iAppUi.CurrentTrack()->Artist().String8(), *this);
	}

CMobblerBrowserControl::~CMobblerBrowserControl()
	{
	if (iBrCtlInterface)
		{
		delete iBrCtlInterface;
		}
	}

void CMobblerBrowserControl::DataL(const TDesC8& aData, CMobblerLastFmConnection::TTransactionError aTransactionError)
	{
	if (aTransactionError == CMobblerLastFmConnection::ETransactionErrorNone)
		{
		_LIT(KHttpDummy, "http://dummy");
		HBufC8* tagsText(NULL);
		HBufC8* similarArtistsText(NULL);
		HBufC8* imageUrl(NULL);
		HBufC8* artistInfo(NULL);

		CMobblerParser::ParseArtistInfoL(aData, artistInfo, imageUrl, tagsText, similarArtistsText);

		CleanupStack::PushL(tagsText);
		CleanupStack::PushL(imageUrl);
		CleanupStack::PushL(artistInfo);
		CleanupStack::PushL(similarArtistsText);

		// Decide how big the artist picture should be taking into account the width
		// of the application
		TRect applicationRect(iAppUi.ApplicationRect());
		TInt artistImageWidth((TInt)((TReal)applicationRect.Width() * 0.45));

		__ASSERT_DEBUG(artistImageWidth < 1000, User::Invariant());

		HBufC8* artistInfoHtml = HBufC8::NewLC(KArtistInfoHtmlTemplate().Length() +
								 iAppUi.CurrentTrack()->Artist().String8().Length() +
								 tagsText->Length() +
								 similarArtistsText->Length() +
								 imageUrl->Length() +
								 3 +
								 artistInfo->Length());

		TPtr8 artistHtmlPtr(artistInfoHtml->Des());
		artistHtmlPtr.AppendFormat(KArtistInfoHtmlTemplate,
									&(iAppUi.CurrentTrack()->Artist().String8()),
									imageUrl,
									artistImageWidth,
									tagsText,
									similarArtistsText,
									artistInfo);
		TDataType dataType(KHtmlDataType());
		TUid uid;
		uid.iUid = KCharacterSetIdentifierUtf8;
		iBrCtlInterface->LoadDataL(KHttpDummy, artistHtmlPtr, dataType, uid);

		CleanupStack::PopAndDestroy(5, tagsText);
		}
	else
		{
		CAknInformationNote* note(new (ELeave) CAknInformationNote(EFalse));
		note->ExecuteLD(static_cast<CMobblerAppUi*>(CCoeEnv::Static()->AppUi())->ResourceReader().ResourceL(R_MOBBLER_ERROR));
		}
	}

void CMobblerBrowserControl::SizeChanged()
	{
	if (iBrCtlInterface)
		{
		iBrCtlInterface->SetRect(Rect());
		}
	//iAppUi.StatusPane()->MakeVisible(EFalse);
	}

TInt CMobblerBrowserControl::CountComponentControls() const
	{
	return 1;
	}

CCoeControl* CMobblerBrowserControl::ComponentControl(TInt aIndex) const
	{
	switch (aIndex)
		{
		case 0:
			return iBrCtlInterface;
		default:
			return NULL;
		}
	}

void CMobblerBrowserControl::HandleResourceChange(TInt aType)
	{
	TRect rect;

	if (aType == KEikDynamicLayoutVariantSwitch)
		{
		AknLayoutUtils::LayoutMetricsRect(AknLayoutUtils::EMainPane, rect);
		SetRect(rect);
		SetExtentToWholeScreen(); // Full screen
		iAppUi.StatusPane()->MakeVisible(ETrue);
		}

	CCoeControl::HandleResourceChange(aType);
	}

void CMobblerBrowserControl::Draw(const TRect& /*aRect*/) const
	{
	CWindowGc& gc(SystemGc());
	gc.Clear(Rect());
	}

TKeyResponse CMobblerBrowserControl::OfferKeyEventL(const TKeyEvent& aKeyEvent, TEventCode aEventCode)
	{
	// This is a hack because the CBrCtlInterface which is part of the
	// platform makes both the left and right arrow keys scroll the page
	// up which is counter-intuitive. So change right to down before passing
	// the key event to the control.
	TKeyEvent newKeyEvent(aKeyEvent);
	if (aKeyEvent.iCode == EKeyRightArrow)
		{
		newKeyEvent.iCode = EKeyDownArrow;
		}

	return iBrCtlInterface->OfferKeyEventL(newKeyEvent, aEventCode);
	}

// End of file
