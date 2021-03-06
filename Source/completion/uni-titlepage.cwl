# uni-titlepage package
# Matthew Bertucci 8/27/2021 for v0.6

#include:scrbase
#include:graphicx

\TitlePageStyle[option=value,...%keyvals]{style}
\maketitle[option=value,...%keyvals]

\TitleOption{option%keyvals}{value}
\TitleOptions{option=value,...%keyvals}

#keyvals:\TitlePageStyle/DHBW
author=
company=
course=
date=
academicgrade=
discipline=
duration=
mainlogo=
matriculationnumber=
place=
referee=
student=
subject=
title=
titlehead=
university=
#endkeyvals

#keyvals:\TitlePageStyle/KOMAScript
author=
date=
dedication=
publisher=
student=
subject=
subtitle=
title=
titlehead=
#endkeyvals

#keyvals:\TitlePageStyle/Markus-1
author=
date=
dedication=
publisher=
student=
subject=
subtitle=
title=
#endkeyvals

#keyvals:\TitlePageStyle/Markus-2
author=
date=
dedication=
publisher=
student=
subject=
subtitle=
title=
titlehead=
#endkeyvals

#keyvals:\TitlePageStyle/Spacer
author=
date=
dedication=
publisher=
student=
subject=
subtitle=
title=
titlehead=
#endkeyvals

#keyvals:\TitlePageStyle/TU-DD
advisor=
author=
chair=
date=
discipline=
faculty=
matriculationnumber=
professor=
student=
subject=
title=
university=
#endkeyvals

#keyvals:\TitlePageStyle/TU-HH
author=
date=
academicgrade=
oralexaminationdate=
place=
referee=
student=
subject=
subtitle=
title=
university=
#endkeyvals

#keyvals:\TitlePageStyle/KIT
author=
chair=
date=
homepage=
mainlogo=
student=
subject=
title=
titlehead=
university=
#endkeyvals

#keyvals:\TitlePageStyle/JT-Aufsaetze
author=
publisher=
student=
subject=
title=
#endkeyvals

#keyvals:\TitlePageStyle/JT-Geschichte
author=
place=
publisher=
student=
subject=
title=
#endkeyvals

#keyvals:\TitlePageStyle/JT-Typography
author=
date=
place=
publisher=
student=
subject=
title=
#endkeyvals

#keyvals:\TitlePageStyle/WWUM
author=
chair=
date=
academicgrade=
discipline=
faculty=
oralexaminationdate=
place=
professor=
sience=
student=
subject=
title=
university=
#endkeyvals

#keyvals:\TitlePageStyle/UKoLa
advisor=
author=
chair=
academicgrade=
mainlogo=
secondlogo=
place=
referee=
student=
subject=
title=
#endkeyvals
 
#keyvals:\maketitle,\TitleOptions
advisor=
author=
chair=
company=
course=
date=
dedication=
academicgrade=
discipline=
duration=
faculty=
homepage=
mainlogo=
secondlogo=
matriculationnumber=
oralexaminationdate=
place=
professor=
publisher=
referee=
sience=
student=
subject=
subtitle=
title=
titlehead=
university=
#endkeyvals

#keyvals:\TitleOption
advisor
author
chair
company
course
date
dedication
academicgrade
discipline
duration
faculty
homepage
mainlogo
secondlogo
matriculationnumber
oralexaminationdate
place
professor
publisher
referee
sience
student
subject
subtitle
title
titlehead
university
#endkeyvals

\NowButAfterBeginDocument{code}#*
\begin{titlepage}[options%keyvals]#*
\end{titlepage}#*
\begin{fullsizetitle}[options%keyvals]#*
\end{fullsizetitle}#*

#keyvals:\begin{titlepage},\begin{fullsizetitle}
pagestyle=
pagenumber=
#endkeyvals

\usetitleelement{element%keyvals}#*

#keyvals:\usetitleelement
advisor
author
chair
company
course
date
dedication
academicgrade,
discipline
duration
faculty
homepage
mainlogo
secondlogo
matriculationnumber
oralexaminationdate
place
professor
publisher
referee
subject
subtitle
title
titlehead
university
#endkeyvals
