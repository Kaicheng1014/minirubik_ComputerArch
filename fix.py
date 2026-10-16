import re

# 讀取你的組合語言檔案
with open('solver.s', 'r') as f:
    content = f.read()

def replacer(match):
    # 將 "\007\006\005" 切割並轉為整數陣列
    nums = [str(int(n)) for n in match.group(1).split('\\') if n]
    return '.byte\t' + ', '.join(nums)

# 替換所有的 .ascii 為 .byte 陣列
new_content = re.sub(r'\.ascii\s+"([^"]+)"', replacer, content)
# 順便清除多餘的 .string ""
new_content = new_content.replace('\t.string\t""\n', '')

# 寫出修復後的檔案
with open('solver_fixed.s', 'w') as f:
    f.write(new_content)

print("轉換完成！請使用 solver_fixed.s")
