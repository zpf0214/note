"启用Pathogen
execute pathogen#infect()
syntax on
filetype plugin indent on
filetype indent on
set nu
set encoding=utf-8
set history=20
set tabstop=4
set softtabstop=4
set shiftwidth=4
set autoindent
set expandtab
set smartcase
set smartindent
set ignorecase
set hlsearch
set backspace=indent,eol,start
set guicursor=n-v-c:block
let g:rustfmt_autosave = 1

"括号补全设置
"没有设置>符号的补全，因为它有不等于的含义
inoremap ' ''<ESC>i
inoremap " ""<ESC>i
inoremap ( ()<ESC>i
inoremap [ []<ESC>i
inoremap } {}<ESC>i<CR><ESC>O
inoremap { {}<ESC>i

"tab out
"设置跳出自动补全的括号
func SkipPair()  
    if getline('.')[col('.') - 1] == ')' || getline('.')[col('.') - 1] == ']' || getline('.')[col('.') - 1] == '"' || getline('.')[col('.') - 1] == "'" || getline('.')[col('.') - 1] == '}'  || getline('.')[col('.') - 1] == '>'
        return "\<ESC>la"  
    else  
        return "\t"  
    endif  
endfunc  
" 将tab键绑定为跳出括号  
inoremap <TAB> <c-r>=SkipPair()<CR>

" autocmd FileType python noremap <buffer> <F8> :call Autopep8()<CR>

command Vter vertical terminal

" 设置颜色主题为 desert
colorscheme default
