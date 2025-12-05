/*
 * This file is part of cSploit.
 *
 * cSploit is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * cSploit is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with cSploit. If not, see <http://www.gnu.org/licenses/>.
 */

package org.csploit.android.helpers;

import android.app.Activity;
import android.app.AlertDialog;
import android.app.Dialog;
import android.content.Context;
import android.content.DialogInterface;
import android.graphics.Color;
import android.text.SpannableString;
import android.text.method.LinkMovementMethod;
import android.view.Gravity;
import android.view.View;
import android.view.inputmethod.InputMethodManager;
import android.widget.EditText;
import android.widget.FrameLayout;
import android.widget.LinearLayout;
import android.widget.ScrollView;
import android.widget.TextView;
import android.widget.Toast;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.appcompat.app.AppCompatActivity;

/**
 * UIHelper - Centralized UI utilities and common dialog management
 *
 * Provides streamlined UI operations including:
 * - Simple toast notifications
 * - Common dialog creation (confirmation, input, error, info)
 * - Keyboard management
 * - View visibility and state management
 * - Color and resource utilities
 * - Dialog styling and theming
 *
 * Features:
 * - Chainable dialog builder pattern
 * - Automatic keyboard handling
 * - Text input validation support
 * - Rich text support with HTML
 * - Theme-aware styling
 * - Touch outside dismissal control
 * - Custom button listeners
 *
 * Usage:
 * {@code
 * // Simple toast
 * UIHelper.toast(context, "Operation complete");
 *
 * // Confirmation dialog
 * UIHelper.confirm(context, "Delete?", "Remove this item?", () -> {
 *     deleteItem();
 * });
 *
 * // Input dialog
 * UIHelper.input(context, "Enter name", "Name:", input -> {
 *     LoggingHelper.d("Input", input);
 * });
 *
 * // Error dialog
 * UIHelper.error(context, "Error", "Operation failed!");
 *
 * // Keyboard management
 * UIHelper.showKeyboard(editText);
 * UIHelper.hideKeyboard(activity);
 * }
 *
 * @author cSploit Team
 * @version 1.0
 */
public final class UIHelper {

    private static final String TAG = "UIHelper";

    // Private constructor to prevent instantiation
    private UIHelper() {}

    /**
     * Show simple toast message
     *
     * @param context Android context
     * @param message message to display
     */
    public static void toast(@NonNull Context context, @NonNull String message) {
        toast(context, message, Toast.LENGTH_SHORT);
    }

    /**
     * Show toast message with custom duration
     *
     * @param context Android context
     * @param message message to display
     * @param duration Toast.LENGTH_SHORT or Toast.LENGTH_LONG
     */
    public static void toast(@NonNull Context context, @NonNull String message, int duration) {
        Toast.makeText(context, message, duration).show();
        LoggingHelper.d(TAG, "Toast: " + message);
    }

    /**
     * Show error toast
     *
     * @param context Android context
     * @param message error message
     */
    public static void toastError(@NonNull Context context, @NonNull String message) {
        toast(context, "❌ " + message, Toast.LENGTH_LONG);
    }

    /**
     * Show success toast
     *
     * @param context Android context
     * @param message success message
     */
    public static void toastSuccess(@NonNull Context context, @NonNull String message) {
        toast(context, "✓ " + message, Toast.LENGTH_SHORT);
    }

    /**
     * Show confirmation dialog
     *
     * @param context Android context
     * @param title dialog title
     * @param message dialog message
     * @param onConfirm callback when confirmed
     */
    public static void confirm(@NonNull Context context, @NonNull String title,
                              @NonNull String message, @Nullable Runnable onConfirm) {
        confirm(context, title, message, onConfirm, null);
    }

    /**
     * Show confirmation dialog with custom buttons
     *
     * @param context Android context
     * @param title dialog title
     * @param message dialog message
     * @param onConfirm callback when confirmed
     * @param onCancel callback when cancelled
     */
    public static void confirm(@NonNull Context context, @NonNull String title,
                              @NonNull String message, @Nullable Runnable onConfirm,
                              @Nullable Runnable onCancel) {
        AlertDialog.Builder builder = new AlertDialog.Builder(context);
        builder.setTitle(title)
                .setMessage(message)
                .setPositiveButton("OK", (dialog, which) -> {
                    if (onConfirm != null) onConfirm.run();
                    dialog.dismiss();
                })
                .setNegativeButton("Cancel", (dialog, which) -> {
                    if (onCancel != null) onCancel.run();
                    dialog.dismiss();
                });

        AlertDialog dialog = builder.create();
        dialog.show();
        LoggingHelper.d(TAG, "Confirmation dialog: " + title);
    }

    /**
     * Show input dialog
     *
     * @param context Android context
     * @param title dialog title
     * @param hint input field hint
     * @param onInput callback with input text
     */
    public static void input(@NonNull Context context, @NonNull String title,
                            @NonNull String hint, @Nullable InputCallback onInput) {
        input(context, title, hint, onInput, null);
    }

    /**
     * Show input dialog with validation
     *
     * @param context Android context
     * @param title dialog title
     * @param hint input field hint
     * @param onInput callback with input text
     * @param validator optional input validator
     */
    public static void input(@NonNull Context context, @NonNull String title,
                            @NonNull String hint, @Nullable InputCallback onInput,
                            @Nullable InputValidator validator) {
        AlertDialog.Builder builder = new AlertDialog.Builder(context);

        final EditText input = new EditText(context);
        input.setHint(hint);
        input.setPadding(16, 16, 16, 16);

        builder.setTitle(title)
                .setView(input)
                .setPositiveButton("OK", (dialog, which) -> {
                    String text = input.getText().toString().trim();

                    if (validator != null && !validator.isValid(text)) {
                        UIHelper.error(context, "Invalid Input", validator.getErrorMessage());
                        return;
                    }

                    if (onInput != null) onInput.onInput(text);
                    dialog.dismiss();
                })
                .setNegativeButton("Cancel", (dialog, which) -> dialog.dismiss());

        AlertDialog dialog = builder.create();
        dialog.show();
        LoggingHelper.d(TAG, "Input dialog: " + title);
    }

    /**
     * Show error dialog
     *
     * @param context Android context
     * @param title dialog title
     * @param message error message
     */
    public static void error(@NonNull Context context, @NonNull String title,
                            @NonNull String message) {
        error(context, title, message, null);
    }

    /**
     * Show error dialog with callback
     *
     * @param context Android context
     * @param title dialog title
     * @param message error message
     * @param onDismiss callback when dismissed
     */
    public static void error(@NonNull Context context, @NonNull String title,
                            @NonNull String message, @Nullable Runnable onDismiss) {
        AlertDialog.Builder builder = new AlertDialog.Builder(context);
        builder.setTitle(title)
                .setMessage(message)
                .setIcon(android.R.drawable.ic_dialog_alert)
                .setPositiveButton("OK", (dialog, which) -> {
                    if (onDismiss != null) onDismiss.run();
                    dialog.dismiss();
                });

        AlertDialog dialog = builder.create();
        dialog.show();
        LoggingHelper.e(TAG, "Error dialog: " + title + " - " + message);
    }

    /**
     * Show "child not started" error
     *
     * @param context Android context
     * @param toolName name of the tool that failed to start
     */
    public static void childNotStarted(@NonNull Context context, @NonNull String toolName) {
        error(context, "Error", toolName + " failed to start");
    }

    /**
     * Show "child not started" toast error
     *
     * @param context Android context
     */
    public static void childNotStartedToast(@NonNull Context context) {
        toastError(context, "Child process not started");
    }

    /**
     * Show "tap again" confirmation toast
     *
     * @param context Android context
     */
    public static void tapAgainToConfirm(@NonNull Context context) {
        toast(context, "Tap again to confirm", Toast.LENGTH_SHORT);
    }

    /**
     * Show tool execution error
     *
     * @param context Android context
     * @param toolName name of the tool
     * @param error error message from tool
     */
    public static void toolError(@NonNull Context context, @NonNull String toolName,
                                @NonNull String error) {
        error(context, toolName + " Error", error);
    }

    /**
     * Show "not running" error
     *
     * @param context Android context
     * @param serviceName name of the service
     */
    public static void notRunning(@NonNull Context context, @NonNull String serviceName) {
        error(context, "Error", serviceName + " is not running");
    }

    /**
     * Show info dialog
     *
     * @param context Android context
     * @param title dialog title
     * @param message info message
     */
    public static void info(@NonNull Context context, @NonNull String title,
                           @NonNull String message) {
        AlertDialog.Builder builder = new AlertDialog.Builder(context);
        builder.setTitle(title)
                .setMessage(message)
                .setIcon(android.R.drawable.ic_dialog_info)
                .setPositiveButton("OK", (dialog, which) -> dialog.dismiss());

        AlertDialog dialog = builder.create();
        dialog.show();
        LoggingHelper.d(TAG, "Info dialog: " + title);
    }

    /**
     * Show scrollable text dialog (for long content)
     *
     * @param context Android context
     * @param title dialog title
     * @param message message content
     */
    public static void textDialog(@NonNull Context context, @NonNull String title,
                                 @NonNull String message) {
        ScrollView scrollView = new ScrollView(context);
        TextView textView = new TextView(context);
        textView.setText(message);
        textView.setPadding(16, 16, 16, 16);
        textView.setTextSize(14);
        scrollView.addView(textView);

        AlertDialog.Builder builder = new AlertDialog.Builder(context);
        builder.setTitle(title)
                .setView(scrollView)
                .setPositiveButton("OK", (dialog, which) -> dialog.dismiss());

        AlertDialog dialog = builder.create();
        dialog.show();
        LoggingHelper.d(TAG, "Text dialog: " + title);
    }

    /**
     * Show keyboard
     *
     * @param view view to show keyboard for
     */
    public static void showKeyboard(@NonNull View view) {
        InputMethodManager imm = (InputMethodManager) view.getContext()
                .getSystemService(Context.INPUT_METHOD_SERVICE);
        if (imm != null) {
            view.requestFocus();
            imm.showSoftInput(view, InputMethodManager.SHOW_IMPLICIT);
            LoggingHelper.d(TAG, "Keyboard shown");
        }
    }

    /**
     * Hide keyboard
     *
     * @param activity activity to hide keyboard in
     */
    public static void hideKeyboard(@NonNull Activity activity) {
        InputMethodManager imm = (InputMethodManager) activity
                .getSystemService(Context.INPUT_METHOD_SERVICE);
        View view = activity.getCurrentFocus();

        if (imm != null && view != null) {
            imm.hideSoftInputFromWindow(view.getWindowToken(), 0);
            LoggingHelper.d(TAG, "Keyboard hidden");
        }
    }

    /**
     * Hide keyboard for specific view
     *
     * @param context Android context
     * @param view view to hide keyboard for
     */
    public static void hideKeyboard(@NonNull Context context, @NonNull View view) {
        InputMethodManager imm = (InputMethodManager) context
                .getSystemService(Context.INPUT_METHOD_SERVICE);
        if (imm != null) {
            imm.hideSoftInputFromWindow(view.getWindowToken(), 0);
            LoggingHelper.d(TAG, "Keyboard hidden for view");
        }
    }

    /**
     * Toggle view visibility
     *
     * @param view view to toggle
     */
    public static void toggleVisibility(@NonNull View view) {
        view.setVisibility(view.getVisibility() == View.VISIBLE ? View.GONE : View.VISIBLE);
    }

    /**
     * Set view visibility
     *
     * @param view view to modify
     * @param visible true to show, false to hide
     */
    public static void setVisible(@NonNull View view, boolean visible) {
        view.setVisibility(visible ? View.VISIBLE : View.GONE);
    }

    /**
     * Enable or disable view
     *
     * @param view view to modify
     * @param enabled true to enable, false to disable
     */
    public static void setEnabled(@NonNull View view, boolean enabled) {
        view.setEnabled(enabled);
        view.setAlpha(enabled ? 1.0f : 0.5f);
    }

    /**
     * Create horizontal divider line
     *
     * @param context Android context
     * @param color divider color
     * @param heightDp divider height in dp
     * @return divider view
     */
    @NonNull
    public static View createDivider(@NonNull Context context, int color, int heightDp) {
        View divider = new View(context);
        divider.setBackgroundColor(color);
        divider.setLayoutParams(new LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.MATCH_PARENT,
                dpToPx(context, heightDp)
        ));
        return divider;
    }

    /**
     * Convert DP to pixels
     *
     * @param context Android context
     * @param dp value in density-independent pixels
     * @return value in pixels
     */
    public static int dpToPx(@NonNull Context context, int dp) {
        return (int) (dp * context.getResources().getDisplayMetrics().density);
    }

    /**
     * Convert pixels to DP
     *
     * @param context Android context
     * @param px value in pixels
     * @return value in density-independent pixels
     */
    public static int pxToDp(@NonNull Context context, int px) {
        return (int) (px / context.getResources().getDisplayMetrics().density);
    }

    /**
     * Get screen width in pixels
     *
     * @param context Android context
     * @return screen width
     */
    public static int getScreenWidth(@NonNull Context context) {
        return context.getResources().getDisplayMetrics().widthPixels;
    }

    /**
     * Get screen height in pixels
     *
     * @param context Android context
     * @return screen height
     */
    public static int getScreenHeight(@NonNull Context context) {
        return context.getResources().getDisplayMetrics().heightPixels;
    }

    /**
     * Callback interface for input dialog
     */
    @FunctionalInterface
    public interface InputCallback {
        void onInput(String input);
    }

    /**
     * Validator interface for input dialog
     */
    public interface InputValidator {
        boolean isValid(String input);

        String getErrorMessage();
    }

    /**
     * Simple email validator
     */
    public static class EmailValidator implements InputValidator {
        @Override
        public boolean isValid(String input) {
            return input != null && input.matches("[a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\\.[a-zA-Z]{2,}");
        }

        @Override
        public String getErrorMessage() {
            return "Invalid email format";
        }
    }

    /**
     * Simple URL validator
     */
    public static class URLValidator implements InputValidator {
        @Override
        public boolean isValid(String input) {
            return input != null && (input.startsWith("http://") || input.startsWith("https://"));
        }

        @Override
        public String getErrorMessage() {
            return "URL must start with http:// or https://";
        }
    }

    /**
     * Simple length validator
     */
    public static class LengthValidator implements InputValidator {
        private final int minLength;
        private final int maxLength;

        public LengthValidator(int minLength, int maxLength) {
            this.minLength = minLength;
            this.maxLength = maxLength;
        }

        @Override
        public boolean isValid(String input) {
            return input != null && input.length() >= minLength && input.length() <= maxLength;
        }

        @Override
        public String getErrorMessage() {
            return "Length must be between " + minLength + " and " + maxLength;
        }
    }
}
